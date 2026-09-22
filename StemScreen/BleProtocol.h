#pragma once
#include <stdint.h>
#include <stddef.h>

constexpr char STEM_SERVICE[] = "93e1b4a0-5c21-4e62-a738-9d608c17f201";
constexpr char STEM_RX[] = "93e1b4a1-5c21-4e62-a738-9d608c17f201";
constexpr size_t STEM_PACKET_SIZE = 20;
constexpr uint8_t CLOCK_VALID = 1, DURATION_VALID = 2, STATE_VALID = 4;
enum class RideStatus : uint8_t { Off = 0, Stopped = 1, AutoPaused = 2, Running = 3 };
struct RidePacket {
  uint8_t flags = 0;
  RideStatus status = RideStatus::Off;
  uint16_t year = 0;
  uint8_t month = 0, day = 0, hour = 0, minute = 0, second = 0;
  uint32_t durationSeconds = 0;
  uint16_t durationMillis = 0; // Fractional part, 0..999.
};
inline uint32_t readLE32(const uint8_t* p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
         (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
inline bool validDate(uint16_t year, uint8_t month, uint8_t day) {
  if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1) return false;
  const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  const unsigned limit = days[month-1] + (month == 2 && year % 4 == 0 ? 1 : 0);
  return day <= limit;
}
inline bool decodeRide(const uint8_t* p, size_t n, RidePacket& out) {
  if (!p || n != STEM_PACKET_SIZE || p[0] != 0x42 || p[1] != 0x53 ||
      p[2] != 3 || p[3] != 2 || (p[4] & ~7u) || p[5] > 3 || p[13]) return false;
  RidePacket decoded;
  decoded.flags = p[4]; decoded.status = static_cast<RideStatus>(p[5]);
  decoded.year = uint16_t(p[6]) | (uint16_t(p[7]) << 8);
  decoded.month = p[8]; decoded.day = p[9]; decoded.hour = p[10];
  decoded.minute = p[11]; decoded.second = p[12];
  decoded.durationSeconds = readLE32(p+14);
  decoded.durationMillis = uint16_t(p[18]) | (uint16_t(p[19]) << 8);
  if (decoded.durationMillis > 999) return false;
  if (decoded.flags & CLOCK_VALID) {
    if (!validDate(decoded.year, decoded.month, decoded.day) ||
        decoded.hour > 23 || decoded.minute > 59 || decoded.second > 59) return false;
  } else {
    for (size_t i = 6; i <= 12; ++i) if (p[i]) return false;
  }
  if (!(decoded.flags & DURATION_VALID) && (decoded.durationSeconds || decoded.durationMillis)) return false;
  if (!(decoded.flags & STATE_VALID) && p[5]) return false;
  out = decoded;
  return true;
}
inline bool packetFresh(bool connected, bool seenThisConnection,
                        uint32_t now, uint32_t received) {
  return connected && seenThisConnection && uint32_t(now-received) < 5000;
}
