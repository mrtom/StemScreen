#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

// Pure logic: no display, Arduino or wall-clock dependency.
struct RideTimer {
  bool running = false;
  uint64_t accumulatedMs = 0;
  uint64_t startedMs = 0;
  uint64_t elapsedMs(uint64_t now) const {
    return accumulatedMs + (running ? now - startedMs : 0);
  }
  void toggle(uint64_t now) {
    if (running) accumulatedMs += now - startedMs;
    else startedMs = now;
    running = !running;
  }
  void reset() { running = false; accumulatedMs = 0; startedMs = 0; }
};

enum class ButtonEvent { None, ShortPress, LongPress };
struct RideButton {
  static constexpr uint64_t DEBOUNCE_MS = 30;
  static constexpr uint64_t LONG_PRESS_MS = 2000;
  bool raw = false, stable = false, longFired = false;
  uint64_t rawChangedMs = 0, pressedMs = 0;
  ButtonEvent update(bool pressed, uint64_t now) {
    if (pressed != raw) { raw = pressed; rawChangedMs = now; }
    if (raw != stable && now - rawChangedMs >= DEBOUNCE_MS) {
      stable = raw;
      if (stable) { pressedMs = now; longFired = false; }
      else if (!longFired) {
        // Measure until the physical release, not its debounce delay.
        const bool held = rawChangedMs - pressedMs >= LONG_PRESS_MS;
        return held ? ButtonEvent::LongPress : ButtonEvent::ShortPress;
      }
    }
    if (stable && raw && !longFired && now - pressedMs >= LONG_PRESS_MS) {
      longFired = true;
      return ButtonEvent::LongPress;
    }
    return ButtonEvent::None;
  }
};

inline void formatRideTime(uint64_t ms, char *out, size_t size) {
  uint64_t seconds = ms / 1000;
  // Keep the requested eight-character display; saturate after 99:59:59.
  if (seconds > 359999) seconds = 359999;
  snprintf(out, size, "%02u:%02u:%02u", unsigned(seconds / 3600),
           unsigned((seconds / 60) % 60), unsigned(seconds % 60));
}

inline bool parseTimeCommand(const char *line, int64_t &epoch) {
  if (strncmp(line, "TIME ", 5) != 0 || !line[5]) return false;
  for (const char *p = line + 5; *p; ++p)
    if (*p < '0' || *p > '9') return false;
  errno = 0;
  char *end;
  const unsigned long long value = strtoull(line + 5, &end, 10);
  if (errno || *end || value < 1704067200ULL || value >= 4102444800ULL)
    return false; // Accept 2024 through 2099 only.
  epoch = static_cast<int64_t>(value);
  return true;
}
