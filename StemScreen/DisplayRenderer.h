#pragma once
#include <Adafruit_GFX.h>
#include <math.h>
#include "RideUi.h"
#include "Battery.h"

constexpr int DISPLAY_WIDTH = 240;
constexpr int DISPLAY_HEIGHT = 240;
constexpr int DISPLAY_BAND_HEIGHT = 48;
constexpr int BATTERY_RING_OUTER_RADIUS = 115;
constexpr int RING_THICKNESS = 4;
constexpr int RING_GAP = 2;
constexpr int STATUS_RING_OUTER_RADIUS = BATTERY_RING_OUTER_RADIUS - RING_THICKNESS - RING_GAP;

inline void drawBatteryRing(GFXcanvas16& canvas, int bandY, const BatteryState& battery,
                            const BatteryRing& ring) {
  // Both rings are four pixels thick, separated by two clear pixels.
  // Half-degree steps overlap at this radius, making a continuous arc.
  canvas.startWrite();
  for (unsigned step = 0; step < 720; ++step) {
    const unsigned degree = step / 2;
    const bool gap = degree % 90 < 2;
    const bool lit = degree < ring.solidDegrees ||
      (ring.flashOn && degree < ring.flashEndDegrees);
    const uint16_t color = gap ? 0x0000 :
      !battery.valid ? 0x7bef : lit ? ring.color : 0x0000;
    const float angle = step * (3.14159265f / 360.0f);
    for (int radius = BATTERY_RING_OUTER_RADIUS - RING_THICKNESS + 1;
         radius <= BATTERY_RING_OUTER_RADIUS; ++radius) {
      canvas.writePixel(120 + lroundf(radius * sinf(angle)),
        120 - lroundf(radius * cosf(angle)) - bandY, color);
    }
  }
  canvas.endWrite();
  // Centre the eight-pixel icon on the battery stroke at 12 o'clock.
  // Draw the black backing after both rings, preserving RING_GAP padding.
  constexpr int iconX = 111, iconWidth = 18, iconHeight = 8;
  const int iconY = 120 - BATTERY_RING_OUTER_RADIUS +
    (RING_THICKNESS - iconHeight) / 2 - bandY;
  canvas.fillRect(iconX - RING_GAP, iconY - RING_GAP,
    iconWidth + 2 * RING_GAP, iconHeight + 2 * RING_GAP, 0x0000);
  // Steady colour does not imply charging detection.
  if (battery.valid && battery.quarters == 4) {
    canvas.fillRect(iconX, iconY, 16, iconHeight, ring.color);
  } else {
    canvas.drawRect(iconX, iconY, 16, iconHeight, ring.color);
    // Twelve interior pixels give exact quarter fills, with a black inset.
    // The shared quarter level keeps the icon and ring hysteresis aligned.
    if (battery.valid && battery.quarters > 0) {
      canvas.fillRect(iconX + 2, iconY + 2, battery.quarters * 3,
        iconHeight - 4, ring.color);
    }
  }
  canvas.fillRect(iconX + 16, iconY + 2, 2, 4, ring.color);
}

inline void drawText(GFXcanvas16& canvas, int bandY, const char* text,
                     int y, int size, uint16_t color) {
  canvas.setTextWrap(false);
  canvas.setTextSize(size);
  canvas.setTextColor(color);
  int16_t x, top; uint16_t w, h;
  canvas.getTextBounds(text, 0, 0, &x, &top, &w, &h);
  canvas.setCursor((DISPLAY_WIDTH-int(w))/2-x, y-bandY);
  canvas.print(text);
}

// All layers are composed offscreen. Only the caller's completed bitmap goes
// to the LCD, so text backgrounds and icon padding never temporarily erase it.
inline void composeBand(GFXcanvas16& canvas, int bandY, const RideScreen& screen,
                        const BatteryState& battery, const BatteryRing& ring) {
  canvas.fillScreen(0x0000);
  drawText(canvas, bandY, screen.title, 44, 2, 0xffff);
  drawText(canvas, bandY, screen.value, 92, screen.valueSize, screen.valueColor);
  drawText(canvas, bandY, screen.detail, 151, strlen(screen.detail) <= 16 ? 2 : 1, screen.valueColor);
  drawText(canvas, bandY, screen.connection, 180, 1, screen.tint);
  drawText(canvas, bandY, screen.footer, 199, 1, 0xffff);
  canvas.fillCircle(111, 215-bandY, 3, screen.ridePage ? 0x7bef : screen.tint);
  canvas.fillCircle(129, 215-bandY, 3, screen.ridePage ? screen.tint : 0x7bef);
  for (int radius = STATUS_RING_OUTER_RADIUS - RING_THICKNESS + 1;
       radius <= STATUS_RING_OUTER_RADIUS; ++radius) {
    canvas.drawCircle(120, 120-bandY, radius, screen.ring);
  }
  drawBatteryRing(canvas, bandY, battery, ring);
}

inline bool sameScreen(const RideScreen& a, const RideScreen& b) {
  return a.valueSize == b.valueSize && a.tint == b.tint && a.ring == b.ring &&
    a.valueColor == b.valueColor && a.ridePage == b.ridePage &&
    strcmp(a.title, b.title) == 0 && strcmp(a.value, b.value) == 0 &&
    strcmp(a.detail, b.detail) == 0 && strcmp(a.connection, b.connection) == 0 &&
    strcmp(a.footer, b.footer) == 0;
}
inline bool sameBatteryImage(const BatteryState& a, const BatteryRing& ar,
                             const BatteryState& b, const BatteryRing& br) {
  return a.valid == b.valid && a.quarters == b.quarters &&
    ar.solidDegrees == br.solidDegrees && ar.flashEndDegrees == br.flashEndDegrees &&
    ar.color == br.color && ar.flashOn == br.flashOn;
}
