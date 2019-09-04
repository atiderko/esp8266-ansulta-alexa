/**************************************************************

This file is a part of
https://github.com/atiderko/esp8266-ansulta-alexa

Licensed under MIT license

 **************************************************************/

#include "AnsultaLightHandler.h"

AnsultaLightHandler::AnsultaLightHandler(Ansulta* ansulta, Config* cfg, OnBoardLED* led) {
    _info.bulbType =  hue::BulbType::DIMMABLE_LIGHT;
    _cfg = cfg;
    _ansulta = ansulta;
    _led = led;
}

String AnsultaLightHandler::getFriendlyName(int lightNumber) const {
    return _cfg->device_name;  // defined in config.h
}

void AnsultaLightHandler::handleQuery(int lightNumber, hue::LightInfo newInfo, JsonObject& raw) {
    int brightness = newInfo.brightness;
    DEBUG_PRINT("ON: ");
    DEBUG_PRINTLN(newInfo.on);
    DEBUG_PRINT("brightness: ");
    DEBUG_PRINTLN(brightness);
    if (newInfo.on) {
        if (brightness == 0 || brightness == 1) {
            brightness = 254;
        }
        DEBUG_PRINT("turn on " + this->getFriendlyName(lightNumber));
        if (_ansulta->get_brightness() > 0 && brightness <= 127) {
            _led->blink(2, 300);
            DEBUG_PRINTLN(" to 50%");
            _ansulta->light_ON_50(50, true, brightness);
        } else {
            _led->blink(3, 300);
            DEBUG_PRINTLN(" to 100%");
            _ansulta->light_ON_100(50, true, brightness);
        }
    } else {
        // switch off
        brightness = 1;
        DEBUG_PRINTLN("turn off " + this->getFriendlyName(lightNumber));
        _led->blink(1, 300);
        _ansulta->light_OFF(50, true, brightness);
    }
    _info.on = newInfo.on;
    _info.brightness = brightness;
}

hue::LightInfo AnsultaLightHandler::getInfo(int lightNumber) {
    _info.on = _ansulta->get_state() != _ansulta->OFF;
    if (_ansulta->get_state() == _ansulta->OFF) {
        _info.brightness = _ansulta->get_brightness();
    } else if (_ansulta->get_state() == _ansulta->ON_50) {
        _info.brightness = _ansulta->get_brightness();
    } else if (_ansulta->get_state() == _ansulta->ON_100) {
        _info.brightness = _ansulta->get_brightness();
    }
    return _info;
}