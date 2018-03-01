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
#ifndef HUELIGHTGROUP_H
#define HUELIGHTGROUP_H

#include <ArduinoJson.h>
#include "HueTypes.h"

namespace hue {

class LightGroup {
  public:
    LightGroup(JsonObject& root);
    bool fillJson(JsonObject& root);
    bool fillSceneJson(JsonObject& root, bool withStates, LightServiceInterface* lightService);
    unsigned int getLightMask();
    // only used for scenes
    String id;
    String getName();

  protected:
    String name;
    // use unsigned int to hold members of this group. 2 bytes -> supports up to 16 lights
    unsigned int lights = 0;
    // no need to hold the group type, only LightGroup is supported for API 1.4
};

};
#endif
