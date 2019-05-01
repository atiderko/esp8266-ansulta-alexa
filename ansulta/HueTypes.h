/**************************************************************

This file is a part of
https://github.com/atiderko/esp8266-ansulta-alexa
Copyright (c) 2018 Alexander Tiderko

Licensed under MIT license

Simulates the Hue Bridge which can be found and control by Alexa.
The code is based on
https://github.com/probonopd/ESP8266HueEmulator
(removed dependency from aJSON, NtpClient, Time, NeoPixelBus)

**************************************************************/
#ifndef HUETYPES_H
#define HUETYPES_H

#include <ArduinoJson.h>
#include "debug.h"

namespace hue {

struct rgbcolor {
  rgbcolor(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {};
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

#define COLOR_SATURATION 255.0f
#define MIN(a,b) ((a<b)?a:b)
#define MAX(a,b) ((a>b)?a:b)
struct hsvcolor {
  hsvcolor(const rgbcolor& color) {
    float r = ((float)color.r)/COLOR_SATURATION;
    float g = ((float)color.g)/COLOR_SATURATION;
    float b = ((float)color.b)/COLOR_SATURATION;
    float mi = MIN(MIN(r,g),b);
    float ma = MAX(MAX(r,g),b);
    float diff = ma - mi;
    v = ma;
    h = 0;
    s = (!v)?0:(diff/ma);
    if (diff) {
      if (r == v) {
            h = (g - b) / diff + (g < b ? 6.0f : 0.0f);
        } else if (g == v) {
            h = (b - r) / diff + 2.0f;
        } else {
            h = (r - g) / diff + 4.0f;
        }
        h /= 6.0f;
    }
  };
  float h;
  float s;
  float v;
};


enum ColorType {
  TYPE_HUE_SAT, TYPE_CT, TYPE_XY
};


enum Alert {
  ALERT_NONE, ALERT_SELECT, ALERT_LSELECT
};


enum Effect {
  EFFECT_NONE, EFFECT_COLORLOOP
};


enum class BulbType {
  EXTENDED_COLOR_LIGHT, DIMMABLE_LIGHT
};


struct LightInfo {
  bool on = false;
  int brightness = 0;
  ColorType type = TYPE_CT;
  BulbType bulbType = BulbType::DIMMABLE_LIGHT;
  int hue = 0;
  int saturation = 0;
  Alert alert = ALERT_NONE;
  Effect effect = EFFECT_NONE;
  unsigned int transitionTime = 400; // by default there is a transition time to the new state of 400 milliseconds
};


class LightHandler {
  public:
    // These functions include light number as a single LightHandler could conceivably service several lights
    virtual void handleQuery(int lightNumber, LightInfo info, JsonObject& raw) {}
    virtual LightInfo getInfo(int lightNumber) {
      LightInfo info;
      return info;
    }
    virtual String getFriendlyName(int lightNumber) const {
      return "Hue Light " + ((String) (lightNumber + 1));
    }
};


class LightServiceInterface {
  public:
    virtual LightHandler* getLightHandler(int numberOfTheLight) {
        return new LightHandler();
    }
};

};
#endif
