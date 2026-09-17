/* Bike Stem Computer: standalone prototype 02.
   Waveshare ESP32-S3-LCD-1.28 NON-TOUCH; same wiring/libraries as Hello.
   Short BOOT press: start/pause. Hold BOOT 2 seconds: clear and pause.
   Pages rotate every 30 seconds; a button action shows Ride Time immediately.
   Build time is approximate. See README for setting accurate time over USB.
*/
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <esp_timer.h>
#include <sys/time.h>
#include <time.h>
#include "RideLogic.h"

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Choose Tools > Board > esp32 > ESP32S3 Dev Module."
#endif

constexpr int LCD_SCK = 10, LCD_MOSI = 11, LCD_CS = 9;
constexpr int LCD_DC = 8, LCD_RST = 12, LCD_BL = 40, RIDE_BUTTON_PIN = 0;
constexpr uint64_t SCREEN_INTERVAL_MS = 10000;
constexpr uint8_t DISPLAY_ROTATION = 0;
static_assert(SCREEN_INTERVAL_MS > 0, "Screen interval must be greater than zero.");
// UK time including the current GMT/BST switching rule.
// The compiler timestamp below is interpreted as UK local time.
constexpr char TIME_ZONE[] = "GMT0BST,M3.5.0/1,M10.5.0/2";

SPIClass displaySPI(HSPI);
Adafruit_GC9A01A display(&displaySPI, LCD_DC, LCD_CS, LCD_RST);
// Compose a complete image in RAM, then transfer it to avoid clearing flicker.
GFXcanvas16 canvas(240, 240);
RideTimer ride;
RideButton button;
bool timerPage = false, forceRedraw = true, clockSynced = false;
uint64_t pageStartedMs = 0;
int64_t lastRenderedValue = -1;
char inputLine[48];
size_t inputLength = 0;
bool discardLine = false;

uint64_t monotonicMs() {
  return static_cast<uint64_t>(esp_timer_get_time()) / 1000;
}

void centredText(const char *text, int16_t y, uint8_t size, uint16_t colour) {
  canvas.setTextSize(size);
  canvas.setTextColor(colour);
  int16_t x, top;
  uint16_t w, h;
  canvas.getTextBounds(text, 0, y, &x, &top, &w, &h);
  canvas.setCursor((240 - w) / 2 - x, y);
  canvas.print(text);
}

bool setClock(int64_t epoch) {
  struct timeval tv = {};
  tv.tv_sec = static_cast<time_t>(epoch);
  return settimeofday(&tv, nullptr) == 0;
}

void setClockFromBuild() {
  // These macros are filled by the compiler on the user's Mac.
  char month[4];
  int day, year, hour, minute, second;
  if (sscanf(__DATE__, "%3s %d %d", month, &day, &year) != 3 ||
      sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second) != 3) return;
  const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  const char *found = strstr(months, month);
  if (!found) return;
  struct tm tm = {};
  tm.tm_year = year - 1900;
  tm.tm_mon = (found - months) / 3;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = second;
  tm.tm_isdst = -1;
  const time_t epoch = mktime(&tm);
  if (epoch > 0) setClock(epoch);
  // Never describe a build timestamp as a live time sync.
  clockSynced = false;
}

void printStatus() {
  time_t epoch = time(nullptr);
  struct tm local;
  localtime_r(&epoch, &local);
  char wall[40], elapsed[16];
  strftime(wall, sizeof(wall), "%Y-%m-%d %H:%M:%S %Z", &local);
  formatRideTime(ride.elapsedMs(monotonicMs()), elapsed, sizeof(elapsed));
  Serial.printf("Clock: %s [%s] | Ride: %s [%s]\n", wall,
                clockSynced ? "USB set" : "approx build time", elapsed,
                ride.running ? "running" : "paused");
}

void handleLine() {
  inputLine[inputLength] = '\0';
  if (inputLength == 0) return;
  if (strcmp(inputLine, "STATUS") == 0) { printStatus(); return; }
  int64_t epoch;
  if (parseTimeCommand(inputLine, epoch) && setClock(epoch)) {
    clockSynced = true;
    forceRedraw = true;
    Serial.println("OK: clock set from USB. Ride timer unchanged.");
    printStatus();
  } else {
    Serial.println("Use TIME <Unix seconds, 2024-2099>, or STATUS. End with newline.");
  }
}

void pollSerial() {
  // Bound each pass so incoming serial data cannot starve the button.
  for (unsigned n = 0; n < 64 && Serial.available(); ++n) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r' || c == '\n') {
      if (discardLine) Serial.println("ERROR: command too long; discarded.");
      else handleLine();
      inputLength = 0;
      discardLine = false;
    } else if (!discardLine) {
      if (inputLength + 1 < sizeof(inputLine)) inputLine[inputLength++] = c;
      else discardLine = true;
    }
  }
}

void drawFrame(uint64_t now) {
  canvas.fillScreen(GC9A01A_BLACK);
  canvas.setTextWrap(false);
  const uint16_t accent = timerPage ? (ride.running ? GC9A01A_GREEN : GC9A01A_ORANGE)
                                    : GC9A01A_CYAN;
  canvas.drawCircle(120, 120, 114, accent);
  if (timerPage) {
    centredText("Ride Time", 54, 2, GC9A01A_WHITE);
    char elapsed[16];
    formatRideTime(ride.elapsedMs(now), elapsed, sizeof(elapsed));
    centredText(elapsed, 105, 4, GC9A01A_WHITE);
    centredText(ride.running ? "RUNNING" : "PAUSED", 153, 2, accent);
    centredText("Tap: start / pause", 181, 1, GC9A01A_WHITE);
    centredText("Hold 2s: reset", 194, 1, GC9A01A_WHITE);
  } else {
    time_t epoch = time(nullptr);
    struct tm local;
    localtime_r(&epoch, &local);
    char clock[8], date[24], zone[12];
    strftime(clock, sizeof(clock), "%H:%M", &local);
    strftime(date, sizeof(date), "%a %d %b", &local);
    strftime(zone, sizeof(zone), "%Z", &local);
    centredText("CLOCK", 45, 2, GC9A01A_WHITE);
    centredText(zone, 70, 1, accent);
    centredText(clock, 94, 6, GC9A01A_WHITE);
    centredText(date, 153, 2, accent);
    centredText(clockSynced ? "Time set from Mac" : "Approx time - set via USB",
                177, 1, clockSynced ? GC9A01A_WHITE : GC9A01A_ORANGE);
    centredText(ride.running ? "Tap: pause ride" : "Tap: start ride", 194, 1,
                GC9A01A_WHITE);
  }
  canvas.drawCircle(111, 215, 3, GC9A01A_DARKGREY);
  canvas.drawCircle(129, 215, 3, GC9A01A_DARKGREY);
  canvas.fillCircle(timerPage ? 129 : 111, 215, 3, accent);
  display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 240);
}

void setup() {
  Serial.begin(115200);
  pinMode(RIDE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, LOW);
  setenv("TZ", TIME_ZONE, 1);
  tzset();
  setClockFromBuild();
  displaySPI.begin(LCD_SCK, -1, LCD_MOSI, LCD_CS);
  display.begin(20000000);
  display.setRotation(DISPLAY_ROTATION);
  display.fillScreen(GC9A01A_BLACK);
  if (!canvas.getBuffer()) {
    // Allocation failure must not silently leave a blank device.
    display.setTextColor(GC9A01A_RED);
    display.setTextSize(2);
    display.setCursor(36, 108);
    display.print("RAM error");
    digitalWrite(LCD_BL, HIGH);
    while (true) { Serial.println("ERROR: screen buffer allocation failed"); delay(1000); }
  }
  pageStartedMs = monotonicMs();
  drawFrame(pageStartedMs);
  digitalWrite(LCD_BL, HIGH);
  // The loop records the first time value on its next pass.
  forceRedraw = true;
  Serial.println("Prototype 02 ready: tap BOOT to start/pause, hold 2s to reset.");
  Serial.println("Clock uses approximate build time. Send TIME <Unix seconds> to set it.");
  printStatus();
}

void loop() {
  const uint64_t now = monotonicMs();
  const ButtonEvent event = button.update(digitalRead(RIDE_BUTTON_PIN) == LOW, now);
  if (event != ButtonEvent::None) {
    if (event == ButtonEvent::LongPress) ride.reset();
    else ride.toggle(now);
    timerPage = true;
    pageStartedMs = now; // Give the action feedback a full page interval.
    forceRedraw = true;
    printStatus();
  }
  pollSerial();
  const uint64_t intervals = (now - pageStartedMs) / SCREEN_INTERVAL_MS;
  if (intervals) {
    if (intervals % 2) timerPage = !timerPage;
    pageStartedMs += intervals * SCREEN_INTERVAL_MS;
    forceRedraw = true;
  }
  const int64_t value = timerPage
      ? static_cast<int64_t>(ride.elapsedMs(now) / 1000)
      : static_cast<int64_t>(time(nullptr) / 60);
  if (forceRedraw || value != lastRenderedValue) {
    drawFrame(now);
    lastRenderedValue = value;
    forceRedraw = false;
  }
  delay(5);
}
