#include "../StemScreen/RideUi.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

int main() {
  // Fixed sender vector: 2026-09-17 14:35:59, RUNNING, 3661 active seconds.
  uint8_t bytes[] = {0x42,0x53,3,2,7,3,0xea,7,9,17,14,35,59,0,0x4d,0x0e,0,0,0,0};
  RidePacket p;
  assert(decodeRide(bytes,20,p));
  assert(p.year==2026 && p.month==9 && p.day==17 && p.hour==14 && p.minute==35 && p.second==59);
  assert(p.durationSeconds==3661 && p.status==RideStatus::Running && p.flags==7);
  for (size_t n=0;n<20;n++) assert(!decodeRide(bytes,n,p));
  assert(!decodeRide(bytes,21,p)); assert(!decodeRide(nullptr,20,p));
  for (unsigned i : {0u,1u,2u,3u,4u,5u,13u,19u}) {
    uint8_t old=bytes[i]; bytes[i]^=0x80;
    assert(!decodeRide(bytes,20,p)); bytes[i]=old;
  }
  bytes[18]=0xe7; bytes[19]=3;
  assert(decodeRide(bytes,20,p) && p.durationMillis==999);
  bytes[18]=0xe8; assert(!decodeRide(bytes,20,p)); // 1000 ms is invalid.
  bytes[18]=bytes[19]=0;
  bytes[2]=2; assert(!decodeRide(bytes,20,p)); bytes[2]=3;
  // Reject the old counter format instead of displaying a counter as ride time.
  uint8_t legacy[] = {0x42,0x53,1,1,1,0,0,0,0,0,0,0,1,0,0,0};
  assert(!decodeRide(legacy,sizeof(legacy),p));
  const unsigned invalidFields[][2] = {{8,0}, {8,13}, {9,0}, {9,32}, {10,24}, {11,60}, {12,60}};
  for (const auto& pair : invalidFields) {
    unsigned offset=pair[0], value=pair[1];
    uint8_t old=bytes[offset]; bytes[offset]=value;
    assert(!decodeRide(bytes,20,p)); bytes[offset]=old;
  }
  assert(validDate(2000,2,29) && validDate(2024,2,29));
  assert(!validDate(2026,2,29) && !validDate(2026,4,31));
  assert(!validDate(1999,12,31) && !validDate(2100,1,1));
  // Unknown data is explicit; absent fields must be zero.
  uint8_t absent[20] = {0x42,0x53,3,2};
  assert(decodeRide(absent,20,p) && p.flags==0);
  absent[14]=1; assert(!decodeRide(absent,20,p)); absent[14]=0;
  absent[6]=1; assert(!decodeRide(absent,20,p)); absent[6]=0;
  absent[18]=1; assert(!decodeRide(absent,20,p)); absent[18]=0;
  absent[5]=1; assert(!decodeRide(absent,20,p)); absent[5]=0;
  // Full unsigned little-endian range; invalid input leaves output unchanged.
  uint8_t saved[4]; memcpy(saved,bytes+14,4); memset(bytes+14,0xff,4);
  assert(decodeRide(bytes,20,p) && p.durationSeconds==0xffffffffu);
  bytes[19]=4; assert(!decodeRide(bytes,20,p) && p.durationSeconds==0xffffffffu);
  bytes[19]=0; memcpy(bytes+14,saved,4);

  ReceiverState receiver;
  assert(linkStatus(receiver,0)==LinkStatus::Disconnected);
  receiver.receive(bytes,19,100);
  assert(!receiver.connected && receiver.packets==0 && receiver.rejected==1);
  // Hardware regression: valid writes arrive without any connect callback.
  receiver.receive(bytes,20,200);
  assert(receiver.connected && receiver.seenThisConnection && receiver.packets==1);
  assert(receiver.whileDisconnected==2 && receiver.connects==0);
  assert(linkStatus(receiver,5199)==LinkStatus::Live);
  receiver.receive(bytes,19,5199);
  assert(receiver.receivedAt==200 && receiver.packets==1);
  assert(linkStatus(receiver,5200)==LinkStatus::Stale);
  receiver.connectionOpened(); receiver.connectionOpened();
  assert(receiver.seenThisConnection && receiver.receivedAt==200);
  receiver.connectionClosed(); receiver.connectionClosed();
  assert(!receiver.connected && !receiver.seenThisConnection && receiver.advertiseAgain);
  assert(linkStatus(receiver,201)==LinkStatus::Disconnected);
  receiver.receive(bytes,19,6000);
  assert(!receiver.connected && receiver.advertiseAgain && receiver.receivedAt==200);
  receiver.connectionOpened();
  assert(linkStatus(receiver,6000)==LinkStatus::WaitingData && !receiver.advertiseAgain);
  receiver.receive(bytes,20,0xfffffff0u);
  assert(linkStatus(receiver,100)==LinkStatus::Live);
  assert(linkStatus(receiver,4984)==LinkStatus::Stale);
  receiver.connectionClosed(); receiver.receive(bytes,20,7000);
  assert(receiver.connected && receiver.seenThisConnection && !receiver.advertiseAgain);

  // Running, manual pause, Auto Pause, resume, activity off and unknown state.
  const char* labels[] = {"NO ACTIVITY","PAUSED","AUTO PAUSED","RUNNING"};
  for (uint8_t status=0;status<4;++status) {
    bytes[5]=status; receiver.receive(bytes,20,8000+status);
    auto screen=makeScreen(receiver,8000+status,true);
    assert(strcmp(screen.detail,labels[status])==0);
    assert(strcmp(screen.value,"01:01:01")==0);
    assert(screen.ring==(status == 0 ? 0x001f : 0x07e0));
  }
  auto clock=makeScreen(receiver,8003,false);
  assert(strcmp(clock.value,"14:35")==0 && strcmp(clock.detail,"17 Sep 2026")==0);
  // Stale/disconnected values freeze and cannot claim current RUNNING state.
  auto stale=makeScreen(receiver,18003,true);
  assert(strcmp(stale.value,"01:01:01")==0 && strcmp(stale.detail,"Last: RUNNING")==0);
  assert(stale.ring==linkColor(LinkStatus::Stale) && stale.valueColor!=0xffff);
  assert(strcmp(makeScreen(receiver,18003,false).value,clock.value)==0);
  receiver.connectionClosed();
  assert(makeScreen(receiver,18004,true).ring==linkColor(LinkStatus::Disconnected));
  // A newly loaded sender supplies Garmin's timer; no local accumulation/reset.
  receiver.receive(bytes,20,20000);
  assert(strcmp(makeScreen(receiver,20000,true).value,"01:01:01")==0);
  receiver.receive(absent,20,21000);
  auto unknown=makeScreen(receiver,21000,true);
  assert(strcmp(unknown.value,"--:--:--")==0 && strcmp(unknown.detail,"STATUS UNKNOWN")==0);
  assert(unknown.tint==0x001f && unknown.ring==0x001f);
  assert(strcmp(makeScreen(receiver,21000,false).value,"--:--")==0);
  bytes[14]=bytes[15]=bytes[16]=bytes[17]=0; bytes[5]=0;
  receiver.receive(bytes,20,22000);
  assert(strcmp(makeScreen(receiver,22000,true).value,"00:00:00")==0);
  // Both pause types blink only the ring, on both pages, at half-second boundaries.
  for (auto status : {RideStatus::Stopped, RideStatus::AutoPaused}) {
    bytes[5]=static_cast<uint8_t>(status);
    receiver.receive(bytes,20,24000);
    for (bool ridePage : {false,true}) {
      for (uint32_t now : {24000u,24499u,24500u,24999u,25000u}) {
        auto screen=makeScreen(receiver,now,ridePage);
        assert(screen.ring==(now>=24500 && now<25000 ? 0x0000 : 0x07e0));
        assert(screen.tint==0x07e0 && screen.valueColor==0xffff);
      }
    }
    receiver.receive(bytes,20,24700); // New data must not restart the flash.
    assert(makeScreen(receiver,24700,true).ring==0x0000);
    assert(makeScreen(receiver,29700,true).ring==0xfd20); // Stale overrides pause.
    receiver.connectionClosed();
    assert(makeScreen(receiver,24701,true).ring==0xf800);
    receiver.connectionOpened();
    assert(makeScreen(receiver,24702,true).ring==0x07ff);
  }
  for (uint8_t status : {0u,3u}) {
    bytes[5]=status; receiver.receive(bytes,20,30000);
    for (uint32_t now : {30000u,30500u,31000u}) {
      assert(makeScreen(receiver,now,true).ring==(status==0 ? 0x001f : 0x07e0));
    }
  }
  // Millisecond arrival anchor: advance between packets, always resync to Garmin.
  bytes[5]=3; bytes[14]=60;
  receiver.receive(bytes,20,40123);
  assert(displayedDuration(receiver,40123)==60);
  assert(displayedDuration(receiver,41122)==60);
  assert(displayedDuration(receiver,41123)==61);
  assert(displayedDuration(receiver,43123)==63);
  assert(strcmp(makeScreen(receiver,43123,true).value,"00:01:03")==0);
  assert(receiver.packet.durationSeconds==60); // Rendering cannot alter the snapshot.
  receiver.receive(bytes,19,43123); // Malformed packets cannot reset the anchor.
  assert(displayedDuration(receiver,44123)==64 && receiver.receivedAt==40123);
  assert(displayedDuration(receiver,45122)==64);
  assert(displayedDuration(receiver,45123)==60); // Stale shows last confirmed value.
  bytes[14]=62; receiver.receive(bytes,20,45130);
  assert(displayedDuration(receiver,45130)==62); // Allow authoritative corrections.
  assert(displayedDuration(receiver,46130)==63);
  for (uint8_t status : {0u,1u,2u}) {
    bytes[5]=status; receiver.receive(bytes,20,47000);
    assert(displayedDuration(receiver,50000)==62);
  }
  bytes[5]=0; bytes[4]=CLOCK_VALID | DURATION_VALID;
  receiver.receive(bytes,20,51000);
  assert(displayedDuration(receiver,54000)==62); // Unknown activity cannot advance.
  bytes[5]=3; bytes[4]=7; receiver.receive(bytes,20,55000);
  assert(displayedDuration(receiver,57000)==64); // Resume.
  receiver.connectionClosed();
  assert(displayedDuration(receiver,57000)==62);
  receiver.connectionOpened();
  assert(displayedDuration(receiver,57000)==62); // Await fresh data on reconnection.
  receiver.receive(bytes,20,0xfffffff0u);
  assert(displayedDuration(receiver,983)==62);
  assert(displayedDuration(receiver,984)==63); // Millisecond rollover.
  assert(displayedDuration(receiver,2984)==65);
  assert(displayedDuration(receiver,4984)==62);
  memset(bytes+14,0xff,4); receiver.receive(bytes,20,60000);
  assert(displayedDuration(receiver,63000)==UINT32_MAX); // Never wrap ride duration.
  bytes[4]=CLOCK_VALID | STATE_VALID; memset(bytes+14,0,4);
  receiver.receive(bytes,20,64000);
  assert(strcmp(makeScreen(receiver,67000,true).value,"--:--:--")==0);
  char duration[24];
  formatDuration(359999,duration,sizeof(duration)); assert(strcmp(duration,"99:59:59")==0);
  formatDuration(360000,duration,sizeof(duration)); assert(strcmp(duration,"100:00:00")==0);
  formatDuration(0xffffffffu,duration,sizeof(duration)); assert(strcmp(duration,"1193046:28:15")==0);
  PageRotation rotation;
  rotation.update(9999); assert(!rotation.ridePage);
  rotation.update(10000); assert(rotation.ridePage);
  rotation.update(40000); assert(!rotation.ridePage);
  rotation.startedAt=0xfffffff0u; rotation.ridePage=false;
  rotation.update(9984); assert(rotation.ridePage);
  puts("Ride protocol, receiver recovery, freshness, UI state and rotation tests passed");
}
