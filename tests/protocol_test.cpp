#include "../StemScreen/BleProtocol.h"
#include "../StemScreen/ReceiverState.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main() {
  // Fixed wire vector: counter=0x12345678, epoch=0x01020304, visible.
  uint8_t bytes[] = {0x42,0x53,1,1,0x78,0x56,0x34,0x12,4,3,2,1,1,0,0,0};
  CounterPacket p = {};
  assert(decodeCounter(bytes,16,p));
  assert(p.counter==0x12345678 && p.epoch==0x01020304 && p.visible);
  for (size_t n=0;n<16;n++) assert(!decodeCounter(bytes,n,p));
  assert(!decodeCounter(bytes,17,p)); assert(!decodeCounter(nullptr,16,p));
  for (unsigned i : {0u,1u,2u,3u,13u,14u,15u}) {
    uint8_t old=bytes[i];bytes[i]^=0x80;
    assert(!decodeCounter(bytes,16,p));bytes[i]=old;
  }
  bytes[12]=2; assert(!decodeCounter(bytes,16,p));
  bytes[12]=0; assert(decodeCounter(bytes,16,p) && !p.visible);
  assert(packetFresh(true,true,4999,0));
  assert(!packetFresh(true,true,5000,0));
  assert(!packetFresh(false,true,1000,0));
  assert(!packetFresh(true,false,1000,0));
  assert(packetFresh(true,true,100,0xfffffff0u)); // millis wrap

  ReceiverState receiver;
  // Malformed traffic must not establish a connection or refresh freshness.
  receiver.receive(bytes, 15, 100);
  assert(!receiver.connected && receiver.packets == 0 && receiver.rejected == 1);
  // Reproduce hardware: valid writes arrive without any onConnect callback.
  receiver.receive(bytes, 16, 200);
  assert(receiver.connected && receiver.seenThisConnection && receiver.packets == 1);
  assert(receiver.packet.counter == 0x12345678 && receiver.receivedAt == 200);
  assert(receiver.whileDisconnected == 2 && receiver.connects == 0);
  assert(packetFresh(receiver.connected, receiver.seenThisConnection, 5199, receiver.receivedAt));
  receiver.receive(bytes, 15, 5199);
  assert(receiver.receivedAt == 200 && receiver.packets == 1);
  assert(!packetFresh(receiver.connected, receiver.seenThisConnection, 5200, receiver.receivedAt));
  // Late/duplicate connection notifications must not erase accepted data.
  receiver.connectionOpened(); receiver.connectionOpened();
  assert(receiver.seenThisConnection && receiver.receivedAt == 200);
  receiver.connectionClosed(); receiver.connectionClosed();
  assert(!receiver.connected && !receiver.seenThisConnection && receiver.advertiseAgain);
  assert(!packetFresh(receiver.connected, receiver.seenThisConnection, 201, receiver.receivedAt));
  receiver.receive(bytes, 15, 6000);
  assert(!receiver.connected && receiver.advertiseAgain && receiver.receivedAt == 200);
  receiver.connectionOpened();
  assert(!receiver.seenThisConnection && !receiver.advertiseAgain);
  receiver.receive(bytes, 16, 0xfffffff0u);
  assert(packetFresh(receiver.connected, receiver.seenThisConnection, 100, receiver.receivedAt));
  assert(!packetFresh(receiver.connected, receiver.seenThisConnection, 4984, receiver.receivedAt));
  // Reconnection inferred from a new write cancels pending advertising restart.
  receiver.connectionClosed();
  receiver.receive(bytes, 16, 7000);
  assert(receiver.connected && receiver.seenThisConnection && !receiver.advertiseAgain);
  puts("Protocol, receiver callback recovery and freshness tests passed");
}
