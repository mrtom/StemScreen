#pragma once
#include "BleProtocol.h"

// A continuous millisecond estimate. Synchronisation preserves its fractional
// phase; only state changes, lost freshness, timer resets or large errors jump.
// Caller owns locking, just as for the received packet. No display/IO work here.
struct RideClock {
  uint64_t anchorMs = 0, lastSourceMs = 0;
  uint32_t anchorAt = 0;
  int32_t correctionMs = 0;
  int8_t errorDirection = 0;
  uint8_t consistentErrors = 0;
  bool running = false;

  uint64_t milliseconds(uint32_t now) const {
    if (!running) return anchorMs;
    const uint32_t elapsed = uint32_t(now - anchorAt);
    // At most 1% rate adjustment (10 ms per second), never a backwards tick.
    const int64_t limit = elapsed / 100;
    const int64_t correction = correctionMs > limit ? limit :
      correctionMs < -limit ? -limit : correctionMs;
    return uint64_t(int64_t(anchorMs) + elapsed + correction);
  }

  void synchronise(const RidePacket& packet, uint32_t now, bool continuous) {
    const uint64_t source = uint64_t(packet.durationSeconds) * 1000 + packet.durationMillis;
    const bool nextRunning = (packet.flags & (DURATION_VALID | STATE_VALID)) ==
      (DURATION_VALID | STATE_VALID) && packet.status == RideStatus::Running;
    const uint64_t estimate = milliseconds(now);
    const int64_t error = int64_t(source) - int64_t(estimate);
    const bool reset = !continuous || !running || !nextRunning || source < lastSourceMs ||
      error >= 2000 || error <= -2000;
    anchorMs = reset ? source : estimate;
    anchorAt = now;
    running = nextRunning;
    lastSourceMs = source;
    if (reset || (error >= -150 && error <= 150)) {
      correctionMs = 0;
      consistentErrors = 0;
      errorDirection = 0;
      return;
    }
    // Ignore isolated arrival jitter. Require three consecutive observations
    // outside the 150 ms deadband, in the same direction, before correcting.
    const int8_t direction = error > 0 ? 1 : -1;
    if (direction != errorDirection) consistentErrors = 0;
    errorDirection = direction;
    if (consistentErrors < 3) ++consistentErrors;
    correctionMs = consistentErrors == 3 ? int32_t(error) : 0;
  }
};
