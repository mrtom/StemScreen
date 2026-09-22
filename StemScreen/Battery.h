#pragma once
#include <stdint.h>

// Waveshare's current non-touch documentation specifies 200k/100k. Verify
// against a meter: its linked schematic instead labels 100k/100k.
constexpr float BATTERY_DIVIDER = 3.0f;
constexpr uint8_t BATTERY_SAMPLE_COUNT = 16;
constexpr uint32_t BATTERY_SAMPLE_MS = 10;
constexpr uint32_t BATTERY_INTERVAL_MS = 5000;

inline uint8_t batteryPercent(uint32_t mv) {
  const uint16_t voltage[] = {3450, 3600, 3700, 3800, 3900, 4000, 4100, 4200};
  const uint8_t percent[] = {0, 10, 20, 40, 60, 75, 90, 100};
  if (mv <= voltage[0]) return 0;
  for (unsigned i = 1; i < sizeof(voltage)/sizeof(voltage[0]); ++i) {
    if (mv < voltage[i]) return percent[i-1] +
      (mv-voltage[i-1]) * (percent[i]-percent[i-1]) / (voltage[i]-voltage[i-1]);
  }
  return 100;
}

// No charger STAT or USB-present GPIO is routed on the documented board.
// Unknown must never be inferred as Charging from a high/rising ADC voltage.
enum class BatteryPower { Unknown, Battery, Charging };
struct BatteryState {
  uint32_t adcMv = 0, millivolts = 0;
  uint8_t percent = 0, quarters = 0;
  bool valid = false;
  BatteryPower power = BatteryPower::Unknown;

  void update(uint32_t averagedAdcMv) {
    adcMv = averagedAdcMv;
    millivolts = uint32_t(adcMv * BATTERY_DIVIDER + 0.5f);
    const bool wasValid = valid;
    valid = millivolts >= 2500 && millivolts <= 4350;
    if (!valid) { percent = 0; quarters = 0; return; }
    percent = batteryPercent(millivolts);
    const uint8_t candidate = percent / 25;
    // Enter each quarter at its threshold, leave 2 percentage points below it.
    if (!wasValid || candidate >= quarters || percent + 2 < quarters * 25)
      quarters = candidate;
  }
};

// One ADC read per scheduled loop iteration; no delays or BLE lock needed.
struct BatterySampler {
  uint32_t lastSample = 0, batchEnded = 0, sum = 0;
  uint8_t count = 0;
  bool waiting = false;
  bool due(uint32_t now) const {
    return uint32_t(now-lastSample) >= BATTERY_SAMPLE_MS &&
      (!waiting || uint32_t(now-batchEnded) >= BATTERY_INTERVAL_MS);
  }
  bool add(uint32_t now, uint32_t adcMv, BatteryState& state) {
    lastSample = now;
    waiting = false;
    sum += adcMv;
    if (++count != BATTERY_SAMPLE_COUNT) return false;
    state.update((sum + BATTERY_SAMPLE_COUNT/2) / BATTERY_SAMPLE_COUNT);
    sum = 0; count = 0; batchEnded = now; waiting = true;
    return true;
  }
};

struct BatteryRing {
  uint16_t solidDegrees = 0, flashEndDegrees = 0, color = 0x8410;
  bool flashOn = false;
};
inline BatteryRing batteryRing(const BatteryState& state, uint16_t linkTint, uint32_t now) {
  BatteryRing ring;
  if (!state.valid) return ring;
  ring.solidDegrees = state.quarters ? state.quarters * 90 : 15;
  ring.color = state.quarters ? linkTint : 0xf800;
  if (state.power == BatteryPower::Charging) {
    ring.color = state.percent < 25 ? 0xf800 : state.percent < 100 ? 0x001f : 0x07e0;
    ring.flashEndDegrees = state.quarters < 4 ? (state.quarters + 1) * 90 : 360;
    // At full there is no next quarter: flash the complete ring green.
    if (state.quarters == 4) ring.solidDegrees = 0;
    ring.flashOn = (now / 500) % 2 == 0;
  }
  return ring;
}
