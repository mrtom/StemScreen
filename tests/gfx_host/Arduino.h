#pragma once
// Minimal host compatibility for Adafruit_GFX's RAM-only canvas tests.
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#define PROGMEM
class __FlashStringHelper;
using String = std::string;
inline float radians(float degrees) { return degrees * 0.017453292519943295f; }
