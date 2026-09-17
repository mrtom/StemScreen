#pragma once
#include <stdint.h>
#include <stddef.h>

constexpr char STEM_SERVICE[] = "93e1b4a0-5c21-4e62-a738-9d608c17f201";
constexpr char STEM_RX[] = "93e1b4a1-5c21-4e62-a738-9d608c17f201";
struct CounterPacket { uint32_t counter; uint32_t epoch; bool visible; };
inline uint32_t readLE32(const uint8_t* p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
         (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
inline bool decodeCounter(const uint8_t* p, size_t n, CounterPacket& out) {
  if (!p || n != 16 || p[0] != 0x42 || p[1] != 0x53 || p[2] != 1 ||
      p[3] != 1 || p[12] > 1 || p[13] || p[14] || p[15]) return false;
  out = {readLE32(p+4), readLE32(p+8), p[12] != 0};
  return true;
}
inline bool packetFresh(bool connected, bool seenThisConnection,
                        uint32_t now, uint32_t received) {
  return connected && seenThisConnection && uint32_t(now-received) < 5000;
}
