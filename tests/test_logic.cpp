#include "../RideLogic.h"
#include <assert.h>
#include <iostream>
int main() {
  RideTimer t;
  assert(!t.running && t.elapsedMs(1000) == 0);
  t.toggle(1000); assert(t.elapsedMs(2250) == 1250);
  t.toggle(2250); assert(!t.running && t.elapsedMs(9000) == 1250);
  t.toggle(9000); assert(t.elapsedMs(10000) == 2250);
  t.reset(); assert(!t.running && t.elapsedMs(15000) == 0);
  t.toggle(0xfffffff0ULL);
  assert(t.elapsedMs(0x1000003d8ULL) == 1000); // No 32-bit millis wrap.
  char buf[16];
  formatRideTime(59999,buf,sizeof(buf)); assert(!strcmp(buf,"00:00:59"));
  formatRideTime(60000,buf,sizeof(buf)); assert(!strcmp(buf,"00:01:00"));
  formatRideTime(3600000,buf,sizeof(buf)); assert(!strcmp(buf,"01:00:00"));
  formatRideTime(360000000,buf,sizeof(buf)); assert(!strcmp(buf,"99:59:59"));
  RideButton b;
  assert(b.update(true,100)==ButtonEvent::None);
  assert(b.update(false,110)==ButtonEvent::None); // Bounce.
  assert(b.update(true,120)==ButtonEvent::None);
  assert(b.update(true,150)==ButtonEvent::None);
  assert(b.update(false,300)==ButtonEvent::None);
  assert(b.update(false,330)==ButtonEvent::ShortPress);
  assert(b.update(false,400)==ButtonEvent::None);
  assert(b.update(true,500)==ButtonEvent::None);
  assert(b.update(true,530)==ButtonEvent::None);
  assert(b.update(true,2529)==ButtonEvent::None);
  assert(b.update(true,2530)==ButtonEvent::LongPress);
  assert(b.update(true,3000)==ButtonEvent::None);
  assert(b.update(false,3100)==ButtonEvent::None);
  assert(b.update(false,3130)==ButtonEvent::None); // Release cannot restart timer.
  RideButton near;
  near.update(true,0); near.update(true,30);
  assert(near.update(false,2029)==ButtonEvent::None);
  assert(near.update(false,2059)==ButtonEvent::ShortPress); // 1999 ms hold.
  int64_t epoch;
  assert(parseTimeCommand("TIME 1788888888",epoch) && epoch==1788888888);
  for (auto s : {"TIME ","TIME -1","TIME 0","TIME 1788888888oops",
                 "TIME 4102444800","TIME 99999999999999999999999999","bad"})
    assert(!parseTimeCommand(s,epoch));
  // Frame text bounding boxes sit within the physical circular aperture.
  auto check = [](int chars,int y,int scale) {
    double halfwidth = (chars*6*scale)/2.0;
    for(double yy: {double(y-120),double(y+8*scale-120)})
      assert(halfwidth*halfwidth+yy*yy < 120.0*120.0);
  };
  check(5,94,6); check(8,105,4); check(10,153,2); check(24,177,1);
  check(17,181,1); check(15,194,1);
  std::cout << "PASS: timer, debounce, long press/release, formatting, serial time parser, text bounds\n";
}
