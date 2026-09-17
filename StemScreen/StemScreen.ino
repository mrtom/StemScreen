// StemScreen ride UI: Garmin local clock and active activity timer.
// Waveshare ESP32-S3-LCD-1.28 NON-TOUCH; Arduino ESP32 core 3.3.0.
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <esp_arduino_version.h>
#include "RideUi.h"

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Select ESP32S3 Dev Module"
#endif
SPIClass displaySPI(HSPI);
Adafruit_GC9A01A display(&displaySPI, 8, 9, 12);
// A small strip buffer keeps redraws smooth without a large BLE RAM cost.
GFXcanvas16 strip(204, 48);
PageRotation rotation;
portMUX_TYPE stateLock = portMUX_INITIALIZER_UNLOCKED;
ReceiverState sharedState;

class ServerEvents : public BLEServerCallbacks {
  void onConnect(BLEServer*) override {
    portENTER_CRITICAL(&stateLock);
    ++sharedState.connects;
    sharedState.connectionOpened();
    portEXIT_CRITICAL(&stateLock);
  }
  void onDisconnect(BLEServer*) override {
    portENTER_CRITICAL(&stateLock);
    ++sharedState.disconnects;
    sharedState.connectionClosed();
    portEXIT_CRITICAL(&stateLock);
  }
};
class WriteEvents : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    const auto value = characteristic->getValue();
    const uint32_t now = millis();
    portENTER_CRITICAL(&stateLock);
    sharedState.receive(reinterpret_cast<const uint8_t*>(value.c_str()), value.length(), now);
    portEXIT_CRITICAL(&stateLock);
  }
};
ServerEvents serverEvents;
WriteEvents writeEvents;

#if defined(CONFIG_BLUEDROID_ENABLED)
// Use the library's hook; do not replace its ESP-IDF callback registration.
void receiverGattEvent(esp_gatts_cb_event_t event, esp_gatt_if_t, esp_ble_gatts_cb_param_t*) {
  if (event != ESP_GATTS_CONNECT_EVT && event != ESP_GATTS_DISCONNECT_EVT) { return; }
  portENTER_CRITICAL(&stateLock);
  if (event == ESP_GATTS_CONNECT_EVT) {
    ++sharedState.rawConnects;
    sharedState.connectionOpened();
  } else {
    ++sharedState.rawDisconnects;
    sharedState.connectionClosed();
  }
  portEXIT_CRITICAL(&stateLock);
}
#endif

void textLine(const char* text, int y, int size, uint16_t color, int height) {
  strip.fillScreen(GC9A01A_BLACK);
  strip.setTextWrap(false);
  strip.setTextSize(size);
  strip.setTextColor(color);
  int16_t x, top; uint16_t w, h;
  strip.getTextBounds(text, 0, 0, &x, &top, &w, &h);
  strip.setCursor((204-int(w))/2-x, 0);
  strip.print(text);
  display.drawRGBBitmap(18, y, strip.getBuffer(), 204, height);
}
void drawScreen(const RideScreen& screen) {
  textLine(screen.title, 44, 2, GC9A01A_WHITE, 24);
  textLine(screen.value, 92, screen.valueSize, screen.valueColor, 48);
  textLine(screen.detail, 151, strlen(screen.detail) <= 16 ? 2 : 1, screen.valueColor, 24);
  textLine(screen.connection, 180, 1, screen.ring, 16);
  textLine(screen.footer, 199, 1, GC9A01A_WHITE, 8);
  display.fillCircle(111, 215, 3, screen.ridePage ? GC9A01A_DARKGREY : screen.ring);
  display.fillCircle(129, 215, 3, screen.ridePage ? screen.ring : GC9A01A_DARKGREY);
  // Redraw after the rectangular strips so the ring remains continuous.
  display.drawCircle(120, 120, 114, screen.ring);
  display.drawCircle(120, 120, 113, screen.ring);
}
void setup() {
  Serial.begin(115200);
  pinMode(40, OUTPUT); digitalWrite(40, LOW);
  displaySPI.begin(10, -1, 11, 9);
  display.begin(20000000);
  display.setRotation(0);
  display.fillScreen(GC9A01A_BLACK);
  digitalWrite(40, HIGH);
  if (!strip.getBuffer()) {
    display.setTextColor(GC9A01A_RED); display.setTextSize(2);
    display.setCursor(35, 110); display.print("RAM error");
    while (true) delay(1000);
  }
  rotation.startedAt = millis();
  drawScreen(makeScreen(sharedState, millis(), false));
  BLEDevice::init("BikeStem");
  Serial.printf("StemScreen ride v2: core=%s address=%s\n",
    ESP_ARDUINO_VERSION_STR, BLEDevice::getAddress().toString().c_str());
  auto* server = BLEDevice::createServer();
  server->setCallbacks(&serverEvents);
#if defined(CONFIG_BLUEDROID_ENABLED)
  BLEDevice::setCustomGattsHandler(receiverGattEvent);
#endif
  auto* service = server->createService(STEM_SERVICE);
  auto* rx = service->createCharacteristic(STEM_RX, BLECharacteristic::PROPERTY_WRITE);
  rx->setCallbacks(&writeEvents);
  service->start();
  auto* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(STEM_SERVICE);
  advertising->setScanResponse(true);
  BLEAdvertisementData response;
  response.setName("BikeStem");
  advertising->setScanResponseData(response);
  advertising->start();
  Serial.println("BikeStem ready: waiting for the Garmin data field.");
}
void loop() {
  static uint32_t lastDraw = 0, loggedPackets = 0, advertiseAt = 0;
  static uint32_t lastHealth = 0, loggedConnects = 0, loggedDisconnects = 0;
  static uint32_t loggedRejected = 0;
  static bool restartPending = false;
  ReceiverState state;
  portENTER_CRITICAL(&stateLock);
  state = sharedState;
  sharedState.advertiseAgain = false;
  portEXIT_CRITICAL(&stateLock);
  const uint32_t now = millis();
  // All logging remains in loop(), outside the BLE callback critical sections.
  if (uint32_t(now-lastHealth) >= 5000 || state.connects != loggedConnects ||
      state.disconnects != loggedDisconnects || state.rejected != loggedRejected) {
    Serial.printf("BLE t=%lu connected=%u connects=%lu disconnects=%lu rawConnects=%lu rawDisconnects=%lu writes=%lu accepted=%lu rejected=%lu malformed=%lu whileDisconnected=%lu lastLen=%u\n",
      (unsigned long)now, unsigned(state.connected), (unsigned long)state.connects,
      (unsigned long)state.disconnects, (unsigned long)state.rawConnects,
      (unsigned long)state.rawDisconnects, (unsigned long)state.writes,
      (unsigned long)state.packets, (unsigned long)state.rejected,
      (unsigned long)state.malformed, (unsigned long)state.whileDisconnected,
      unsigned(state.lastWriteLength));
    lastHealth = now; loggedConnects = state.connects;
    loggedDisconnects = state.disconnects; loggedRejected = state.rejected;
  }
  if (state.advertiseAgain) { restartPending = true; advertiseAt = now; }
  if (state.connected) restartPending = false;
  if (restartPending && uint32_t(now-advertiseAt) >= 250) {
    BLEDevice::startAdvertising(); restartPending = false;
    Serial.println("BLE advertising restart requested");
  }
  if (state.packets != loggedPackets) {
    Serial.printf("RX ride=%lus state=%u flags=%u local=%04u-%02u-%02u %02u:%02u:%02u packets=%lu rejected=%lu\n",
      (unsigned long)state.packet.durationSeconds, unsigned(state.packet.status), unsigned(state.packet.flags),
      unsigned(state.packet.year), unsigned(state.packet.month), unsigned(state.packet.day),
      unsigned(state.packet.hour), unsigned(state.packet.minute), unsigned(state.packet.second),
      (unsigned long)state.packets, (unsigned long)state.rejected);
    loggedPackets = state.packets;
  }
  if (uint32_t(now-lastDraw) >= 250) {
    lastDraw = now;
    rotation.update(now);
    drawScreen(makeScreen(state, now, rotation.ridePage));
  }
  delay(5);
}
