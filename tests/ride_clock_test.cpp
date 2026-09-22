#include "../StemScreen/RideUi.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

static void receive(ReceiverState& state, uint64_t durationMs, uint32_t now,
                    RideStatus status = RideStatus::Running, uint8_t flags = 6) {
  uint8_t wire[20] = {0x42,0x53,3,2,flags,static_cast<uint8_t>(status)};
  const uint32_t seconds = durationMs / 1000;
  for (unsigned i=0;i<4;++i) wire[14+i] = seconds >> (8*i);
  wire[18] = durationMs % 1000;
  wire[19] = (durationMs % 1000) >> 8;
  state.receive(wire, sizeof(wire), now);
}

int main() {
  ReceiverState state;
  // Fractional source phase survives irregular arrivals, including 3-second gaps.
  receive(state,60800,0);
  const uint32_t arrivals[] = {900,2300,5300,6100,8000,10900,13000};
  const uint32_t delays[] = {40,100,20,80,0,120,30};
  unsigned next = 0, ticks = 0;
  uint32_t previous = 60, lastTick = 0;
  for (uint32_t now=0;now<=15000;++now) {
    if (next<7 && now==arrivals[next]) {
      receive(state,60800+now-delays[next],now);
      ++next;
    }
    const auto value=displayedDuration(state,now);
    assert(value==(60800+now)/1000);
    if (value!=previous) {
      if (ticks) assert(now-lastTick==1000);
      else assert(now==200);
      lastTick=now; previous=value; ++ticks;
    }
  }
  assert(ticks==15);
  assert(state.packet.durationSeconds==73 && state.packet.durationMillis==770);
  assert(displayedDuration(state,18000)==73); // Stale returns confirmed value.
  receive(state,79025,19000); // Reacquisition snaps even if the UI wasn't drawn.
  assert(state.rideClock.milliseconds(19000)==79025);

  // Persistent errors slew in either direction, at no more than 1% per interval.
  for (int direction : {-1,1}) {
    ReceiverState drift;
    receive(drift,10000,0);
    for (uint32_t now=1000;now<=3000;now+=1000) {
      receive(drift,10000+now+direction*500,now);
      assert(drift.rideClock.milliseconds(now)==10000+now);
    }
    assert(drift.rideClock.milliseconds(4000)==uint64_t(14000+direction*10));
    // Continued measurements converge, instead of repeatedly jumping.
    for (uint32_t now=4000;now<=50000;now+=1000) {
      const auto before=drift.rideClock.milliseconds(now);
      receive(drift,10000+now+direction*500,now);
      assert(drift.rideClock.milliseconds(now)==before);
    }
    const int64_t error=int64_t(60000+direction*500)-int64_t(drift.rideClock.milliseconds(50000));
    assert(error>=-150 && error<=150);
  }
  // Alternating jitter outside the deadband is not persistent drift.
  ReceiverState jitter;
  receive(jitter,10000,0);
  for (uint32_t now=1000;now<=10000;now+=1000) {
    receive(jitter,10000+now+(now/1000%2 ? 300 : -300),now);
    assert(jitter.rideClock.correctionMs==0);
    assert(jitter.rideClock.milliseconds(now)==10000+now);
  }
  // Pause, Auto Pause, resume, reset, unknown state and missing duration.
  for (auto status : {RideStatus::Stopped,RideStatus::AutoPaused,RideStatus::Off}) {
    receive(state,80075,20000,status);
    assert(state.rideClock.milliseconds(22000)==80075);
    assert(displayedDuration(state,22000)==80);
    receive(state,80500,23000);
    assert(state.rideClock.milliseconds(23000)==80500);
    assert(displayedDuration(state,23500)==81);
  }
  receive(state,100,24000); // Timer reset even with no observed Off packet.
  assert(state.rideClock.milliseconds(24000)==100);
  receive(state,3500,25000); // Large forward discrepancy snaps.
  assert(state.rideClock.milliseconds(25000)==3500);
  receive(state,4000,26000,RideStatus::Off,DURATION_VALID);
  assert(displayedDuration(state,28000)==4);
  receive(state,0,29000,RideStatus::Running,STATE_VALID);
  assert(state.rideClock.milliseconds(30000)==0);
  receive(state,4500,31000);
  state.connectionClosed();
  assert(displayedDuration(state,32000)==4);
  state.connectionOpened();
  assert(displayedDuration(state,32000)==4);
  receive(state,5050,32000);
  assert(state.rideClock.milliseconds(32000)==5050);
  receive(state,60800,0xfffffff0u);
  assert(displayedDuration(state,184)==61); // Fractional tick across millis wrap.
  receive(state,61760,984);
  assert(state.rideClock.milliseconds(984)==61800);
  puts("Continuous ride clock: jitter, drift, state changes and rollover tests passed");
}
