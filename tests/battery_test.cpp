#include <assert.h>
#include <stdio.h>
#include <initializer_list>
#include "../StemScreen/Battery.h"
int main() {
  assert(batteryPercent(0) == 0);
  assert(batteryPercent(3450) == 0);
  assert(batteryPercent(3525) == 5);
  assert(batteryPercent(3600) == 10);
  assert(batteryPercent(3700) == 20);
  assert(batteryPercent(3750) == 30);
  assert(batteryPercent(3800) == 40);
  assert(batteryPercent(3900) == 60);
  assert(batteryPercent(4000) == 75);
  assert(batteryPercent(4100) == 90);
  assert(batteryPercent(4200) == 100);
  assert(batteryPercent(UINT32_MAX) == 100);
  for (unsigned mv = 1; mv <= 4500; ++mv)
    assert(batteryPercent(mv) >= batteryPercent(mv-1));
  BatteryState b;
  assert(batteryRing(b, 0x07e0, 0).solidDegrees == 0);
  b.update(1400);
  assert(b.valid && b.millivolts == 4200 && b.percent == 100 && b.quarters == 4);
  assert(b.power == BatteryPower::Unknown);
  b.update(1390); // 97%: leave full only outside hysteresis.
  assert(b.quarters == 3);
  b.update(1242); // 25% enters first quarter.
  assert(b.quarters == 1);
  b.update(1239); // 23% holds first quarter.
  assert(b.quarters == 1);
  b.update(1235); // 21% becomes low.
  assert(b.quarters == 0);
  auto r = batteryRing(b, 0x07e0, 0);
  assert(r.solidDegrees == 15 && r.color == 0xf800 && !r.flashOn);
  b.power = BatteryPower::Charging;
  r = batteryRing(b, 0x07e0, 0);
  assert(r.solidDegrees == 15 && r.flashEndDegrees == 90 && r.color == 0xf800 && r.flashOn);
  assert(!batteryRing(b, 0x07e0, 500).flashOn);
  b.update(1300); // 60%: two solid quarters, third flashing blue.
  r = batteryRing(b, 0xf800, 1000);
  assert(r.solidDegrees == 180 && r.flashEndDegrees == 270 && r.color == 0x001f && r.flashOn);
  b.update(1400);
  r = batteryRing(b, 0xf800, 1000);
  assert(r.solidDegrees == 0 && r.flashEndDegrees == 360 && r.color == 0x07e0);
  b.power = BatteryPower::Battery;
  for (uint16_t tint : {uint16_t(0x07e0), uint16_t(0x001f), uint16_t(0xfd20)}) {
    r = batteryRing(b, tint, 500);
    assert(r.color == tint && r.solidDegrees == 360 && !r.flashOn);
  }
  b.update(2100); // Wrong divider/revision: don't silently show 100%.
  assert(!b.valid && batteryRing(b, 0xffff, 0).solidDegrees == 0);
  b.update(0);
  assert(!b.valid);
  BatterySampler sampler;
  assert(!sampler.due(9) && sampler.due(10));
  for (unsigned i = 1; i <= 16; ++i)
    assert(sampler.add(i*10, i%2 ? 1390 : 1410, b) == (i == 16));
  assert(b.valid && b.adcMv == 1400 && b.percent == 100);
  assert(!sampler.due(5159) && sampler.due(5160));
  sampler.lastSample = UINT32_MAX-5;
  sampler.batchEnded = UINT32_MAX-4995;
  assert(!sampler.due(3) && sampler.due(4));
  puts("Battery tests passed");
}
