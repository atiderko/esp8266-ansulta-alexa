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
#include "HueLightGroup.h"
#include "HueTypes.h"

using namespace hue;

LightGroup::LightGroup(JsonObject& root)
{
    String tmpName = root["name"];
    this->name = tmpName;
    JsonArray& jLights = root["lights"];
    for (size_t i = 0; i < jLights.size(); i++) {
        // lights are 1-based and map to the 0-based bitfield
        int lightNum = jLights[i];
        if (lightNum != 0) {
            lights |= (1 << (lightNum - 1));
        }
    }
}

bool LightGroup::fillJson(JsonObject& root) {
    root["name"] = name;
    JsonObject& state = root.createNestedObject("state");
    state["all_on"] = "true";
    state["any_on"] = "true";
    JsonArray& data = root.createNestedArray("lights");
    for (int i = 0; i < 16; i++) {
        if (!((1 << i) & lights)) {
            continue;
        }
        // add light to list
        String lightNum = "";
        lightNum += (i + 1);
        data.add(lightNum);
    }
    root["type"] = "LightGroup";
    return root.success();
}


bool LightGroup::fillSceneJson(JsonObject& root, bool withStates, LightServiceInterface* lightService) {
    root["name"] = name;
    JsonArray& jlights = root.createNestedArray("lights");
    for (int i = 0; i < 16; i++) {
        if (!((1 << i) & lights)) {
            continue;
        }
        // add light to list
        String lightNum = "";
        lightNum += (i + 1);
        jlights.add(lightNum);
    }
    root["owner"] = "api";
    root["recycle"] = false;
    root["locked"] = false;
    JsonObject& appdata = root.createNestedObject("appdata");
    appdata["version"] = 1;
    appdata["data"] = "";
    root["picture"] = "";
    root["lastupdated"] = "2017-11-04T10:17:15";
    root["version"] = 2;

    if(withStates==true && lightService)
    {
      DEBUG_PRINTLN("Adding lightstates");
      JsonObject& lightstates = root.createNestedObject("lightstates");
      for (int i = 0; i < 16; i++) {
        if (!((1 << i) & lights)) {
          continue;
        }        
        // add light to list
        String lightNum = "";
        lightNum += (i + 1);
        LightHandler *handler = lightService->getLightHandler(i);
        LightInfo currentInfo = handler->getInfo(i);
        JsonObject& lightState = lightstates.createNestedObject(lightNum);
        lightState["on"] = currentInfo.on;
        lightState["bri"] = currentInfo.brightness;
        JsonArray& xy = lightState.createNestedArray("xy");
        xy.add(0.5806);
        xy.add(0.3903);
      }        
    }

    //String output;
    //root.printTo(output);
    //DEBUG_PRINTLN(output);
    return root.success();
}

unsigned int LightGroup::getLightMask()
{
  return lights;
}

String LightGroup::getName()
{
  return name;  
}

