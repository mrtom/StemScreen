#pragma once
#include "BleProtocol.h"

// Caller owns synchronization. Only a valid packet can refresh receivedAt.
struct ReceiverState {
  CounterPacket packet = {};
  uint32_t receivedAt = 0, packets = 0, rejected = 0;
  uint32_t connects = 0, disconnects = 0, writes = 0;
  uint32_t rawConnects = 0, rawDisconnects = 0;
  uint32_t malformed = 0, whileDisconnected = 0;
  size_t lastWriteLength = 0;
  bool connected = false, seenThisConnection = false, advertiseAgain = false;

  void connectionOpened() {
    // The high-level and raw hooks may both report the same event, or arrive
    // after a write established the connection. Do not erase accepted data.
    if (!connected) { seenThisConnection = false; }
    connected = true;
    advertiseAgain = false;
  }
  void connectionClosed() {
    connected = false;
    seenThisConnection = false;
    advertiseAgain = true;
  }
  void receive(const uint8_t* bytes, size_t length, uint32_t now) {
    ++writes;
    lastWriteLength = length;
    if (!connected) { ++whileDisconnected; }
    CounterPacket decoded;
    if (!decodeCounter(bytes, length, decoded)) {
      ++malformed; ++rejected;
      return;
    }
    // A real GATT write is connection evidence even if onConnect was missed.
    connectionOpened();
    packet = decoded;
    receivedAt = now;
    seenThisConnection = true;
    ++packets;
  }
};
