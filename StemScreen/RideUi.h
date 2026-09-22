#pragma once
#include <stdio.h>
#include <string.h>
#include "ReceiverState.h"

constexpr uint32_t SCREEN_INTERVAL_MS = 10000;
enum class LinkStatus { Disconnected, WaitingData, Live, Stale };
inline LinkStatus linkStatus(const ReceiverState& s, uint32_t now) {
  if (!s.connected) return LinkStatus::Disconnected;
  if (!s.seenThisConnection) return LinkStatus::WaitingData;
  return packetFresh(true, true, now, s.receivedAt) ? LinkStatus::Live : LinkStatus::Stale;
}
inline uint16_t linkColor(LinkStatus status) {
  switch (status) {
    case LinkStatus::Live: return 0x001f;         // blue until activity is confirmed
    case LinkStatus::WaitingData: return 0x07ff;  // cyan
    case LinkStatus::Stale: return 0xfd20;        // orange
    default: return 0xf800;                      // red
  }
}
inline const char* rideStatusText(const RidePacket& p) {
  if (!(p.flags & STATE_VALID)) return "STATUS UNKNOWN";
  switch (p.status) {
    case RideStatus::Running: return "RUNNING";
    case RideStatus::Stopped: return "PAUSED";
    case RideStatus::AutoPaused: return "AUTO PAUSED";
    default: return "NO ACTIVITY";
  }
}
inline void formatDuration(uint32_t s, char* out, size_t n) {
  // Keep duration truthful even beyond 99 hours; use smaller text for long values.
  snprintf(out, n, "%02lu:%02lu:%02lu", (unsigned long)(s/3600),
    (unsigned long)((s/60)%60), (unsigned long)(s%60));
}
struct PageRotation {
  bool ridePage = false;
  uint32_t startedAt = 0;
  void update(uint32_t now) {
    const uint32_t intervals = uint32_t(now-startedAt) / SCREEN_INTERVAL_MS;
    if (intervals) {
      if (intervals % 2) ridePage = !ridePage;
      startedAt += intervals * SCREEN_INTERVAL_MS;
    }
  }
};
struct RideScreen {
  char title[16] = {}, value[24] = {}, detail[32] = {}, connection[32] = {}, footer[32] = {};
  uint8_t valueSize = 4;
  uint16_t tint = 0xf800, ring = 0xf800, valueColor = 0xffff;
  bool ridePage = false;
};
inline RideScreen makeScreen(const ReceiverState& state, uint32_t now, bool ridePage) {
  RideScreen screen;
  screen.ridePage = ridePage;
  const auto link = linkStatus(state, now);
  const bool live = link == LinkStatus::Live;
  const bool seen = state.packets != 0;
  const RidePacket& p = state.packet;
  const bool active = live && (p.flags & STATE_VALID) && p.status != RideStatus::Off;
  const bool paused = active && (p.status == RideStatus::Stopped || p.status == RideStatus::AutoPaused);
  screen.tint = active ? 0x07e0 : linkColor(link);
  // Use the display clock, not packet arrival: regular packets must not reset blinking.
  screen.ring = paused && (now / 500) % 2 ? 0x0000 : screen.tint;
  screen.valueColor = live ? 0xffff : 0x8410;
  snprintf(screen.title, sizeof(screen.title), "%s", ridePage ? "RIDE TIME" : "CLOCK");
  if (ridePage) {
    if (seen && (p.flags & DURATION_VALID)) formatDuration(p.durationSeconds, screen.value, sizeof(screen.value));
    else snprintf(screen.value, sizeof(screen.value), "--:--:--");
    const char* status = seen ? rideStatusText(p) : "WAITING FOR DATA";
    snprintf(screen.detail, sizeof(screen.detail), "%s%s", seen && !live ? "Last: " : "", status);
    screen.valueSize = strlen(screen.value) <= 8 ? 4 : strlen(screen.value) <= 11 ? 3 : 2;
  } else {
    screen.valueSize = 6;
    if (seen && (p.flags & CLOCK_VALID)) {
      snprintf(screen.value, sizeof(screen.value), "%02u:%02u", unsigned(p.hour), unsigned(p.minute));
      const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
      snprintf(screen.detail, sizeof(screen.detail), "%02u %s %04u", unsigned(p.day), months[p.month-1], unsigned(p.year));
    } else {
      snprintf(screen.value, sizeof(screen.value), "--:--");
      snprintf(screen.detail, sizeof(screen.detail), "DATE UNAVAILABLE");
    }
  }
  const char* connection = link == LinkStatus::Live ? "LIVE FROM GARMIN" :
    link == LinkStatus::WaitingData ? "CONNECTED - WAIT DATA" :
    link == LinkStatus::Stale ? "STALE - LAST VALUE" : "GARMIN DISCONNECTED";
  snprintf(screen.connection, sizeof(screen.connection), "%s", connection);
  if (!seen) snprintf(screen.footer, sizeof(screen.footer), "Open StemScreen on Garmin");
  else if (!live) snprintf(screen.footer, sizeof(screen.footer), "Last packet %lus ago", (unsigned long)(uint32_t(now-state.receivedAt)/1000));
  else snprintf(screen.footer, sizeof(screen.footer), "%s", ridePage ? "Excludes pauses" : "Garmin local time");
  return screen;
}
