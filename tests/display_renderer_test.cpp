#include <assert.h>
#include <stdio.h>
#include "../StemScreen/DisplayRenderer.h"

// Compare every pixel of the transmitted bands to a complete offscreen frame:
// catches clipped text/circles at band boundaries and missing overlay layers.
void checkBands(const RideScreen& screen, const BatteryState& battery, uint32_t now) {
  GFXcanvas16 full(240, 240), band(240, 48);
  const auto ring = batteryRing(battery, screen.tint, now);
  composeBand(full, 0, screen, battery, ring);
  for (int y = 0; y < 240; y += 48) {
    band.fillScreen(0xffff); // Previous contents must not leak into this band.
    composeBand(band, y, screen, battery, ring);
    assert(memcmp(full.getBuffer()+y*240, band.getBuffer(), 240*48*2) == 0);
  }
  // Icon padding and the portion of status circle that text used to erase.
  assert(full.getPixel(110, 6) == 0);
  assert(full.getPixel(129, 6) == 0);
  assert(full.getPixel(111, 3) == ring.color);
  assert(full.getPixel(28, 64) == screen.ring);
}
int main() {
  ReceiverState state;
  BatteryState battery;
  auto screen = makeScreen(state, 0, false);
  checkBands(screen, battery, 0);
  for (unsigned quarter = 0; quarter <= 4; ++quarter) {
    battery.valid = true; battery.quarters = quarter;
    battery.percent = quarter * 25;
    for (unsigned page = 0; page < 2; ++page) {
      screen = makeScreen(state, 0, page);
      strcpy(screen.value, page ? "12:34:56" : "12:34");
      screen.tint = screen.ring = 0x07e0;
      checkBands(screen, battery, 0);
      screen.ring = 0; // Paused black phase must not erase the battery.
      checkBands(screen, battery, 500);
      battery.power = BatteryPower::Charging;
      checkBands(screen, battery, 0);
      checkBands(screen, battery, 500);
      battery.power = BatteryPower::Unknown;
    }
  }
  assert(sameScreen(screen, screen));
  auto changed = screen;
  strcpy(changed.value, "12:34:57");
  assert(!sameScreen(screen, changed));
  changed = screen; changed.ring = 0xffff;
  assert(!sameScreen(screen, changed));
  changed = screen; changed.ridePage = !screen.ridePage;
  assert(!sameScreen(screen, changed));
  auto ring = batteryRing(battery, screen.tint, 0);
  auto updated = battery;
  updated.adcMv += 1; updated.millivolts += 3;
  assert(sameBatteryImage(battery, ring, updated, ring));
  updated.quarters = 0;
  assert(!sameBatteryImage(battery, ring, updated, ring));
  auto flash = ring; flash.flashOn = !ring.flashOn;
  assert(!sameBatteryImage(battery, ring, battery, flash));
  puts("Display bands match full frame; overlays, animation and redraw checks passed");
}
