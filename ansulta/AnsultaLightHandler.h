/**************************************************************

This file is a part of
https://github.com/atiderko/esp8266-ansulta-alexa

Licensed under MIT license

 **************************************************************/
#ifndef ANSULTALIGHTHANDLER_H
#define ANSULTALIGHTHANDLER_H

#include "debug.h"
#include "config.h"
#include "OnBoardLED.h"
#include "HueTypes.h"
#include "HueLightService.h"

// Handler used by LightServiceClass to switch the ansulta lights
class AnsultaLightHandler : public hue::LightHandler {
  private:
    hue::LightInfo _info;
    Config* _cfg;
    OnBoardLED* _led;
    Ansulta* _ansulta;

  public:
    AnsultaLightHandler(Ansulta* ansulta, Config* cfg, OnBoardLED* led);
    String getFriendlyName(int lightNumber) const;
    void handleQuery(int lightNumber, hue::LightInfo newInfo, JsonObject& raw);
    hue::LightInfo getInfo(int lightNumber);
};
