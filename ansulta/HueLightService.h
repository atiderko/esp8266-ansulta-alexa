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
#ifndef HUELIGHTSERVICE_H
#define HUELIGHTSERVICE_H

#include <Arduino.h>
#include <ESP8266WebServer.h>

#include "debug.h"
#include "HueTypes.h"
#include "HueWcFnRequestHandler.h"
#include "HueLightGroup.h"

namespace hue {

#define MAX_LIGHT_HANDLERS 16
#define MAX_LIGHT_GROUPS 16
#define COLOR_SATURATION 255.0f
#define WEB_PORT 80

class LightServiceClass : public LightServiceInterface {

public:
    LightServiceClass(int numberOfLights=MAX_LIGHT_HANDLERS);
    ~LightServiceClass();
    LightHandler* getLightHandler(int numberOfTheLight);
    int getLightsAvailable();
    bool setLightsAvailable(int lights);
    bool setLightHandler(int index, LightHandler& handler);
    void begin();
    void begin(ESP8266WebServer *svr);
    void update();
    void ntp_available(bool state);
protected:
    int pCurrentNumLights;
    static LightHandler* pLightHandlers[MAX_LIGHT_HANDLERS]; // interfaces exposed to the outside world
    static LightGroup* pLightGroups[MAX_LIGHT_GROUPS];
    static LightGroup* pLightScenes[MAX_LIGHT_GROUPS];
    ESP8266WebServer *HTTP;
    String friendlyName;
    String bridgeIDString;
    String macString;
    String ipString;
    String netmaskString;
    String gatewayString;
    bool ntpSet;

    void on(WcFnHandlerFunction fn, const String &wcUri, HTTPMethod method, char wildcard = '*');
    
    String listFiles(String _template);
    void clearFiles(String _template);

    void indexPageFn();
    void cacheClearFn();
    void descriptionFn();
    void unimpFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method);
    String generateTargetPutResponse(JsonObject &body, String targetBase);
    
    void addConfigJson(JsonObject& config);
    void sendJson(String msg);
    void sendJson(JsonObject& config);
    void sendJson(JsonArray& config);
    void sendError(int type, String path, String description);
    void sendSuccess(String name, String value);
    void sendSuccess(String value);
    void sendUpdated();

    void configFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method);
    void authFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method);
    bool getGroupJson(JsonObject& root);
    bool getSceneJson(JsonObject& root);
    void addLightsJson(JsonObject& lights);
    void addLightJson(JsonObject& root, int numberOfTheLight, LightHandler* lightHandler);
    void wholeConfigFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method);
    void sceneListingHandler();
    int findSceneIndex(String id);
    bool validateGroupCreateBody(JsonObject& root);
    bool updateSceneSlot(int slot, String id, String body);
    void sceneCreationHandler(String id);
    String scenePutHandler(String id);
    void scenesFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method);
    LightGroup *findScene(String id);
    void scenesIdFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method);
    void scenesIdLightFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method);
    void groupListingHandler();
    bool updateGroupSlot(int slot, String body);
    void groupCreationHandler();
    void groupsFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method);
    void groupsIdFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method);
    bool parseHueLightInfo(LightInfo currentInfo, JsonObject& root, LightInfo *newInfo);
    void applyConfigToLightMask(unsigned int lights);
    void groupsIdActionFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method);
    void lightsFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method);
    void addSingleLightJson(JsonObject& root, int numberOfTheLight, LightHandler* lightHandler);
    void lightsIdFn(WcFnRequestHandler *whandler, String requestUri, HTTPMethod method);
    void lightsIdStateFn(WcFnRequestHandler *whandler, String requestUri, HTTPMethod method);
    void lightsNewFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method);
    rgbcolor getXYtoRGB(float x, float y, int brightness_raw);
    int getHue(hsvcolor hsb);
    int getSaturation(hsvcolor hsb);
    rgbcolor getMirektoRGB(int mirek);
    void initializeGroupSlots();
    void initializeSceneSlots();
};
  
};

#endif
