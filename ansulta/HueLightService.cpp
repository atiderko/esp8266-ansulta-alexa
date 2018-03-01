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
#include "HueLightService.h"
#include <ESP8266WiFi.h>
#include "HueWcFnRequestHandler.h"
#include "HueTemplates.h"

#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <WiFiUdp.h>
#include "SSDP.h"
#include <ArduinoJson.h>
#include <FS.h>

using namespace hue;


LightHandler* LightServiceClass::pLightHandlers[MAX_LIGHT_HANDLERS] = {}; // interfaces exposed to the outside world
LightGroup* LightServiceClass::pLightGroups[MAX_LIGHT_GROUPS] = {nullptr, };
LightGroup* LightServiceClass::pLightScenes[MAX_LIGHT_GROUPS] = {nullptr, };


LightServiceClass::LightServiceClass(int numberOfLights)
{
    pCurrentNumLights = MAX_LIGHT_HANDLERS;
    if (numberOfLights <= MAX_LIGHT_HANDLERS) {
      pCurrentNumLights = numberOfLights;
    }
    HTTP = NULL;
}

LightServiceClass::~LightServiceClass() {
    if (HTTP != NULL) {
        delete HTTP;
    }
    // TODO: delete pLightHandlers
}

bool LightServiceClass::setLightHandler(int index, LightHandler& handler) {
  if (index >= pCurrentNumLights || index < 0) return false;
  pLightHandlers[index] = &handler;
  return true;
}

bool LightServiceClass::setLightsAvailable(int lights) {
  if (lights <= MAX_LIGHT_HANDLERS) {
    pCurrentNumLights = lights;
    return true;
  }
  return false;
}

int LightServiceClass::getLightsAvailable() {
  return pCurrentNumLights;
}

LightHandler* LightServiceClass::getLightHandler(int numberOfTheLight) {
  if (numberOfTheLight >= pCurrentNumLights || numberOfTheLight < 0) {
    return nullptr;
  }

  // this should not happen
  if (!pLightHandlers[numberOfTheLight]) {
    return new LightHandler();
  }

  return pLightHandlers[numberOfTheLight];
}

void LightServiceClass::begin() {
    begin(new ESP8266WebServer(WEB_PORT));
}

String StringIPaddress(IPAddress myaddr)
{
    String LocalIP = "";
    for (int i = 0; i < 4; i++) {
        LocalIP += String(myaddr[i]);
        if (i < 3)
            LocalIP += ".";
    }
    return LocalIP;
}

void LightServiceClass::begin(ESP8266WebServer *svr) {
  bool init_groups = true;
  if (HTTP != NULL) {
    delete HTTP;
    init_groups = false;
  }
  HTTP = svr;
  macString = WiFi.macAddress();
  bridgeIDString = macString;
  bridgeIDString.replace(":", "");
  bridgeIDString = bridgeIDString.substring(0, 6) + "FFFE" + bridgeIDString.substring(6);
  ipString = StringIPaddress(WiFi.localIP());
  netmaskString = StringIPaddress(WiFi.subnetMask());
  gatewayString = StringIPaddress(WiFi.gatewayIP());

  DEBUG_PRINT("Starting HTTP at ");
  DEBUG_PRINT(ipString);
  DEBUG_PRINT(":");
  DEBUG_PRINTLN(WEB_PORT);

  HTTP->on("/index.html", HTTP_GET, std::bind(&LightServiceClass::indexPageFn, this));
  HTTP->on("/cache/clear", HTTP_GET, std::bind(&LightServiceClass::cacheClearFn, this));
  HTTP->on("/description.xml", HTTP_GET, std::bind(&LightServiceClass::descriptionFn, this));
  on(std::bind(&LightServiceClass::configFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/config", HTTP_ANY);
  on(std::bind(&LightServiceClass::configFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/config", HTTP_GET);
  on(std::bind(&LightServiceClass::wholeConfigFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*", HTTP_GET);
  on(std::bind(&LightServiceClass::wholeConfigFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/", HTTP_GET);
  on(std::bind(&LightServiceClass::authFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api", HTTP_POST);
  on(std::bind(&LightServiceClass::unimpFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/schedules", HTTP_GET);
  on(std::bind(&LightServiceClass::unimpFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/rules", HTTP_GET);
  on(std::bind(&LightServiceClass::unimpFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/sensors", HTTP_GET);
  on(std::bind(&LightServiceClass::scenesFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/scenes", HTTP_ANY);
  on(std::bind(&LightServiceClass::scenesIdFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/scenes/*", HTTP_ANY);
  on(std::bind(&LightServiceClass::scenesIdLightFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/scenes/*/lightstates/*", HTTP_ANY);
  on(std::bind(&LightServiceClass::scenesIdLightFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/scenes/*/lights/*/state", HTTP_ANY);
  on(std::bind(&LightServiceClass::groupsFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/groups", HTTP_ANY);
  on(std::bind(&LightServiceClass::groupsIdFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/groups/*", HTTP_ANY);
  on(std::bind(&LightServiceClass::groupsIdActionFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/groups/*/action", HTTP_ANY);
  on(std::bind(&LightServiceClass::lightsFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/lights", HTTP_ANY);
  on(std::bind(&LightServiceClass::lightsNewFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/lights/new", HTTP_ANY);
  on(std::bind(&LightServiceClass::lightsIdFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/lights/*", HTTP_ANY);
  on(std::bind(&LightServiceClass::lightsIdStateFn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "/api/*/lights/*/state", HTTP_ANY);

  HTTP->begin();

  String serial = macString;
  serial.toLowerCase();
  
  DEBUG_PRINTLN("Starting SSDP...");
  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(WEB_PORT);
  SSDP.setName("Philips hue clone");
  SSDP.setSerialNumber(serial.c_str());
  SSDP.setURL("index.html");
  SSDP.setModelName("IpBridge");
  SSDP.setModelNumber("0.1");
  SSDP.setModelURL("http://www.meethue.com");
  SSDP.setManufacturer("Royal Philips Electronics");
  SSDP.setManufacturerURL("http://www.philips.com");

  //SSDP.setDeviceType((char*)"upnp:rootdevice");
  SSDP.setDeviceType("urn:schemas-upnp-org:device:basic:1");
  //SSDP.setMessageFormatCallback(ssdpMsgFormatCallback);
  SSDP.begin();
  DEBUG_PRINTLN("SSDP Started");
  DEBUG_PRINTLN("FS Starting");
  SPIFFS.begin();
  if (init_groups) {
    initializeGroupSlots();
    initializeSceneSlots();
  }
}

void LightServiceClass::update() {
  HTTP->handleClient();
}

void LightServiceClass::ntp_available(bool state)
{
  ntpSet = state;
  time_t rawtime;
  struct tm * timeinfo;
  char buffer [80];
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(buffer, 80, "Now it's %FT:%T.", timeinfo);
  DEBUG_PRINTLN(buffer);
}

void LightServiceClass::on(WcFnHandlerFunction fn, const String &wcUri, HTTPMethod method, char wildcard) {
    HTTP->addHandler(new WcFnRequestHandler(fn, wcUri, method, wildcard));
}

String LightServiceClass::listFiles(String _template)
{
    DEBUG_PRINTLN(_template);
    String response = "";
    for (int i = 0; i < 16; i++) {
        String fileName = _template;
        fileName.replace("%d",String(i));
        DEBUG_PRINTLN(fileName);
      
        //sprintf(fileName,_template,i);
        if(!SPIFFS.exists(fileName))
            continue;
        DEBUG_PRINTLN("Adding file to list");
        response += "<li>" + fileName + "</li>";
    }
    return response;
}

void LightServiceClass::clearFiles(String _template)
{
    for (int i = 0; i < 16; i++) {
        String fileName = _template;
        fileName.replace("%d",String(i));
        if(SPIFFS.exists(fileName)){
            SPIFFS.remove(fileName);   
        }
    }
}

void LightServiceClass::indexPageFn() {
	String response = "<html><body>"
	"<h2>Philips HUE ( {ip} )</h2>"
	"<p>Available lights:</p>"
	"<ul>{lights}</ul>"
  "<ul>File Cache</ul>"
  "<ul>Groups:</ul>"
  "<ul>{group-files}</ul>"
  "<ul>Scenes:</ul>"
  "<ul>{scene-files}</ul>"
  "<a href='/cache/clear'>Clear Cached Groups and Scenes</a>"
	"</body></html>";
	
	String lights = "";
	for (int i = 0; i < this->getLightsAvailable(); i++) {
	    if (!pLightHandlers[i]) {
            continue;
	    }
	    lights += "<li>" + pLightHandlers[i]->getFriendlyName(i) + "</li>";
	}
    String sceneFiles = listFiles(SCENE_FILE_TEMPLATE);
    String groupFiles = listFiles(GROUP_FILE_TEMPLATE);
	  response.replace("{ip}", ipString);
	  response.replace("{lights}", lights);
	  response.replace("{group-files}", groupFiles);	
    response.replace("{scene-files}", sceneFiles);  
	  HTTP->send(200, "text/html", response);
}

// remove the group and scene cache files
void LightServiceClass::cacheClearFn()
{
    clearFiles(GROUP_FILE_TEMPLATE);
    clearFiles(SCENE_FILE_TEMPLATE);
    indexPageFn();
}

void LightServiceClass::descriptionFn()
{
  String response = SSDP_XML_TEMPLATE;

  String escapedMac = macString;
  escapedMac.replace(":", "");
  escapedMac.toLowerCase();
  
  response.replace("{mac}", escapedMac);
  response.replace("{ip}", ipString);
  String port(WEB_PORT);
  response.replace("{port}", port);

  HTTP->send(200, "text/xml", response);
  DEBUG_PRINTLN(response);
}

void LightServiceClass::unimpFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method)
{
  String str = "{}";
  HTTP->send(200, "text/plain", str);
  DEBUG_PRINTLN(str);
}

// targetBase is assumed to have a trailing slash (/)
String LightServiceClass::generateTargetPutResponse(JsonObject &body, String targetBase)
{
    // example: [{"success":{"/lights/1/state/hue":254}}]
    DynamicJsonBuffer jsonBuffer;
    JsonArray& root = jsonBuffer.createArray();
    for (JsonObject::iterator it=body.begin(); it!=body.end(); ++it) {
        String target = targetBase + it->key;
        JsonObject& root_x = root.createNestedObject();
        JsonObject& success = root_x.createNestedObject("success");
        success[target] = it->value;
    }
    String msg;
    root.printTo(msg);
    return msg;
}

void LightServiceClass::addConfigJson(JsonObject& root)
{
    root["name"] = "hue emulator";
    root["swversion"] = "81012917";
    root["bridgeid"]  = bridgeIDString;
    root["portalservices"] = false;
    root["linkbutton"] = true;
    root["mac"] = macString;
    root["dhcp"] = true;
    root["ipaddress"] = ipString;
    root["netmask"] = netmaskString;
    root["gateway"] = gatewayString;
    root["apiversion"] = "1.3.0";
    if (ntpSet) {
//      string (19..19) ISO8601:2004
//      [YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]
//      Example: 2013-12-31T14:12:45
        time_t rawtime;
        struct tm * timeinfo;
        char buffer [80];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, 80, "Now it's %FT:%T.", timeinfo);
        String dts(buffer);  // NTP.getTimeStr(moment);
    }

    // TODO: take this from the settings, once we have spiffs support
    root["timezone"] = "Europe/Berlin";
    JsonObject& whitelist = root.createNestedObject("whitelist");
    JsonObject& whitelistApi = whitelist.createNestedObject("api");
    whitelistApi["name"] = "clientname#devicename";
    JsonObject& swupdate = root.createNestedObject("swupdate");
    swupdate["text"] = "";
    swupdate["notify"] = false;  // Otherwise client app shows update notice
    swupdate["updatestate"] = 0;
    swupdate["url"] = "";
}

void LightServiceClass::sendJson(JsonObject& root)
{
    // Take JsonObject and print it to Serial and to WiFi
    String msg;
    root.printTo(msg);
    DEBUG_PRINT(millis());
    DEBUG_PRINT(": ");
    DEBUG_PRINTLN(msg);
    HTTP->send(200, "application/json", msg);
}

void LightServiceClass::sendJson(JsonArray& root)
{
    // Take JsonObject and print it to Serial and to WiFi
    String msg;
    root.printTo(msg);
    DEBUG_PRINT(millis());
    DEBUG_PRINT(": ");
    DEBUG_PRINTLN(msg);
    HTTP->send(200, "application/json", msg);
}

void LightServiceClass::sendJson(String msg)
{
    // Take string and print it to Serial and to WiFi
    DEBUG_PRINT(millis());
    DEBUG_PRINT(": ");
    DEBUG_PRINTLN(msg);
    HTTP->send(200, "application/json", msg);
}

void LightServiceClass::sendError(int type, String path, String description) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonObject& errorObject = root.createNestedObject("error");
    errorObject["type"] = type;
    errorObject["address"] = path;
    errorObject["description"] = description;
    sendJson(root);
}

void LightServiceClass::sendSuccess(String id, String value) {
    DynamicJsonBuffer jsonBuffer;
    JsonArray& root = jsonBuffer.createArray();
    JsonObject& root_0 = root.createNestedObject();
    JsonObject& root_0_success = root_0.createNestedObject("success");
    root_0_success[id] = value;
    sendJson(root);
}

void LightServiceClass::sendSuccess(String value) {
    DynamicJsonBuffer jsonBuffer;
    JsonArray& root = jsonBuffer.createArray();
    JsonObject& success = root.createNestedObject();
    success["success"] = value;
    sendJson(root);
}

void LightServiceClass::sendUpdated() {
    DEBUG_PRINTLN("Updated.");
    HTTP->send(200, "text/plain", "Updated.");
}

void LightServiceClass::configFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method)
{
    switch (method) {
        case HTTP_GET: {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.createObject();
            addConfigJson(root);
            sendJson(root);
            break;
        }
        case HTTP_PUT: {
            DEBUG_PRINT("configFn:");
            DEBUG_PRINTLN(HTTP->arg("plain"));
            DynamicJsonBuffer jsonBuffer;
            // Parse JSON object
            JsonObject& body = jsonBuffer.parseObject(HTTP->arg("plain"));
            if (body.success()) {
                sendJson(generateTargetPutResponse(body, "/config/"));
                //aJson.deleteItem(body);
                // TODO: actually store this
            }
            break;
        }
        default:
            sendError(4, requestUri, "Config method not supported");
            break;
    }
}


void LightServiceClass::authFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method)
{
    // On the real bridge, the link button on the bridge must have been recently pressed for the command to execute successfully.
    // We try to execute successfully regardless of a button for now.
    sendSuccess("username", "api");
}

bool LightServiceClass::getGroupJson(JsonObject& root)
{
    // iterate over groups and serialize
    for (int i = 0; i < 16; i++) {
        if (pLightGroups[i]) {
            String sIndex = "";
            sIndex += (i + 1);
            JsonObject& lightGroup = root.createNestedObject(sIndex);
            pLightGroups[i]->fillJson(lightGroup);
        }
    }
    return root.success();
}

bool LightServiceClass::getSceneJson(JsonObject& root)
{
    // iterate over scenes and serialize
    for (int i = 0; i < 16; i++) {
        if (pLightScenes[i]) {
            DEBUG_PRINT("Returning Scene :");
            DEBUG_PRINTLN(pLightScenes[i]->id.c_str());
            JsonObject& lightScene = root.createNestedObject(pLightScenes[i]->id);
            pLightScenes[i]->fillSceneJson(lightScene, true, this);
        }
    }
    return root.success();
}

void LightServiceClass::addLightsJson(JsonObject& lights)
{
    DEBUG_PRINTLN("add lights to json, count: " + String(getLightsAvailable()));
    for (int i = 0; i < getLightsAvailable(); i++) {
        addLightJson(lights, i, getLightHandler(i));
    }
}

void LightServiceClass::addLightJson(JsonObject& root, int numberOfTheLight, LightHandler* lightHandler)
{
    if (!lightHandler) 
        return;

    DEBUG_PRINTLN("add light to json, nr: " + String(numberOfTheLight));
    String lightNumber = (String) (numberOfTheLight + 1);
    String lightName = lightHandler->getFriendlyName(numberOfTheLight);
    JsonObject& light = root.createNestedObject(lightNumber);

    LightInfo info = lightHandler->getInfo(numberOfTheLight);
    if (info.bulbType == BulbType::DIMMABLE_LIGHT) {
        light["type"] = "Dimmable light";
    } else {
        light["type"] = "Extended color light";
    }

    light["manufacturername"] = "OpenSource";  // type of lamp (all "Extended colour light" for now)
    light["swversion"] = "0.1";
    light["name"] = lightName;  // the name as set through the web UI or app
    light["uniqueid"] = macString + "-" + (String) (numberOfTheLight + 1);
    light["modelid"] = "LST001";  // the model number

    JsonObject& state = light.createNestedObject("state");
    state["on"] = info.on;
    state["bri"] = info.brightness;  // brightness between 0-254 (NB 0 is not off!)

    if (info.bulbType == BulbType::EXTENDED_COLOR_LIGHT) {
        JsonArray& xy = state.createNestedArray("xy");  // xy mode: CIE 1931 color co-ordinates
        xy.add(0.0);
        xy.add(0.0);
        state["colormode"] = "hs";  // the current color mode
        state["effect"] = info.effect == EFFECT_COLORLOOP ? "colorloop" : "none";
        state["ct"] = 500;  // ct mode: color temp (expressed in mireds range 154-500)
        state["hue"] = info.hue;  // hs mode: the hue (expressed in ~deg*182.04)
        state["sat"] = info.saturation;  // hs mode: saturation between 0-254
    }

    state["alert"] = "none";  // 'select' flash the lamp once, 'lselect' repeat flash for 30s
    state["reachable"] = true;  // lamp can be seen by the hub
}

void LightServiceClass::wholeConfigFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method)
{
    // DEBUG_PRINTLN("Respond with complete json as in https://github.com/probonopd/ESP8266HueEmulator/wiki/Hue-API#get-all-information-about-the-bridge");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonObject& lights = root.createNestedObject("lights");
    addLightsJson(lights);
    JsonObject& groups = root.createNestedObject("groups");
    getGroupJson(groups);
    JsonObject& config = root.createNestedObject("config");
    addConfigJson(config);
    JsonObject& schedules = root.createNestedObject("schedules");
    JsonObject& scenes = root.createNestedObject("scenes");
    getSceneJson(scenes);
    JsonObject& rules = root.createNestedObject("rules");
    JsonObject& sensors = root.createNestedObject("sensors");
    JsonObject& resourcelinks = root.createNestedObject("resourcelinks");
    sendJson(root);
}

void LightServiceClass::sceneListingHandler()
{
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    getSceneJson(root);
    sendJson(root);
}

int LightServiceClass::findSceneIndex(String id)
{
    int index = -1;
    for (int i = 0; i < 16; i++) {
        LightGroup *scene = pLightScenes[i];
        if (scene) {
            if (scene->id == id) {
                return i;
            }
        } else if (index == -1) {
            index = i;
        }
    }
    return index;
}

bool LightServiceClass::validateGroupCreateBody(JsonObject& root) {
    return root.containsKey("name") && root.containsKey("lights");
}

bool LightServiceClass::updateSceneSlot(int slot, String id, String body)
{
    if (body == "") {
        return false;
    }
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(body);
    if (!root.success() || !validateGroupCreateBody(root)) {
        // throw error bad body
        sendError(2, "scenes/" + (slot + 1), "Bad JSON body");
        return false;
    } 
    DEBUG_PRINT("updateSceneSlot:");
    DEBUG_PRINTLN(body);
    if (pLightScenes[slot]) {
        String fileName = SCENE_FILE_TEMPLATE;
        fileName.replace("%d", String(slot));
        delete pLightScenes[slot];
        pLightScenes[slot] = nullptr;
        if (SPIFFS.exists(fileName)){
            SPIFFS.remove(fileName);   
        }
    }
    pLightScenes[slot] = new LightGroup(root);
    return true;
}

void saveToFile(String fileName, JsonObject &root)
{
    File file = SPIFFS.open(fileName, "w");
    if (!file) {
        DEBUG_PRINTLN("Failed to create file: " + fileName);
        return;
    }
    // Serialize JSON to file
    if (root.printTo(file) == 0) {
        DEBUG_PRINTLN("Failed to write to file: " + fileName);
    }
    // Close the file (File's destructor doesn't close the file)
    file.close();
}

void LightServiceClass::sceneCreationHandler(String id)
{
    DEBUG_PRINT("sceneCreationHandler() ");
    DEBUG_PRINTLN(id);
    int sceneIndex = findSceneIndex(id);
    // handle scene creation
    // find first available scene slot
    if (sceneIndex == -1) {
        // throw error no new scenes allowed
        sendError(301, "scenes", "Scenes table full");
        return;
    }
    // updateSceneSlot sends failure messages
    if (updateSceneSlot(sceneIndex, id, HTTP->arg("plain"))) {
        id = String(sceneIndex,DEC);
        // TODO - add file saves here
        DEBUG_PRINT("updating lightScene->id to ");
        DEBUG_PRINTLN(id);
        pLightScenes[sceneIndex]->id = id;
        sendSuccess("id", id);
        // now save to file
        String fileName = SCENE_FILE_TEMPLATE;
        fileName.replace("%d", String(sceneIndex));
        DEBUG_PRINT("Updating Scene ");
        DEBUG_PRINT(id);
        DEBUG_PRINTLN(fileName);
        DynamicJsonBuffer jsonBuffer;
        // Parse the root object
        JsonObject &root = jsonBuffer.createObject();
        pLightScenes[sceneIndex]->fillSceneJson(root, true, this);
        saveToFile(fileName, root);
    }
}

String LightServiceClass::scenePutHandler(String id)
{
    DEBUG_PRINT("sceneCreationHandler() ");
    DEBUG_PRINTLN(id);
    int sceneIndex = findSceneIndex(id);
    // handle scene creation
    // find first available scene slot
    if (sceneIndex == -1) {
        // throw error no new scenes allowed
        sendError(301, "scenes", "Scenes table full");
        return "";
    }
    // updateSceneSlot sends failure messages
    if (updateSceneSlot(sceneIndex, id, HTTP->arg("plain"))) {
        id = String(sceneIndex, DEC);
        // TODO - add file saves here
        DEBUG_PRINT("updating lightScene->id to ");
        DEBUG_PRINTLN(id);
        pLightScenes[sceneIndex]->id = id;

        DynamicJsonBuffer jsonBuffer;
        JsonArray& root = jsonBuffer.createArray();
        JsonObject& success1 = root.createNestedObject();
        JsonObject& success2 = root.createNestedObject();
        JsonObject& addr1 = success1.createNestedObject("success");
        JsonObject& addr2 = success1.createNestedObject("success");
        char addressBuffer[30];
        sprintf(addressBuffer,"/scenes/%d/name",sceneIndex);
        addr1["address"] = addressBuffer;
        addr1["value"] = pLightScenes[sceneIndex]->getName();
        sprintf(addressBuffer,"/scenes/%d/lights",sceneIndex);
        addr2["address"] = addressBuffer;
        JsonArray& lights = addr2.createNestedArray("value");
        for (int i = 0; i < 16; i++) {
            if (!((1 << i) & pLightScenes[sceneIndex]->getLightMask())) {
                continue;
            }
            // add light to list
            String lightNum = "";
            lightNum += (i + 1);
            lights.add(lightNum.c_str());
        }
        sendJson(root);
        // now save to file
        // reuse buffer
        jsonBuffer.clear();
        JsonObject& save_root = jsonBuffer.createObject();
        String fileName = SCENE_FILE_TEMPLATE;
        fileName.replace("%d", String(sceneIndex));
        DEBUG_PRINT("Updating Scene ");
        DEBUG_PRINT(id);
        DEBUG_PRINTLN(fileName);
        pLightScenes[sceneIndex]->fillSceneJson(save_root, true, this);
        saveToFile(fileName, save_root);
    }
    return id;
}

void LightServiceClass::scenesFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method)
{
    String id = "";
    switch (method) {
        case HTTP_GET:
            sceneListingHandler();
            break;
        case HTTP_PUT:
            id = scenePutHandler("");
            break;
        case HTTP_POST:
            sceneCreationHandler("");
            break;
        default:
            sendError(4, requestUri, "Scene method not supported");
            break;
    }
}

LightGroup *LightServiceClass::findScene(String id)
{
    for (int i = 0; i < 16; i++) {
        LightGroup *scene = pLightScenes[i];
        if (scene) {
            if (scene->id == id) {
                return scene;
            }
        }
    }
    return nullptr;
}

void LightServiceClass::scenesIdFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method)
{
    String sceneId = handler->getWildCard(1);
    LightGroup *scene = findScene(sceneId);
    switch (method) {
        case HTTP_GET:
            if (scene) {
                DynamicJsonBuffer jsonBuffer;
                JsonObject& root = jsonBuffer.createObject();
                scene->fillSceneJson(root, true, this);
                sendJson(root);
            } else {
                sendError(3, "/scenes/"+sceneId, "Cannot retrieve scene that does not exist");
            }
            break;
        case HTTP_PUT:
            // validate body, delete old group, create new group
            sceneCreationHandler(sceneId);
            // XXX not a valid response according to API
            sendUpdated();
            break;
        case HTTP_DELETE:
            if (scene) {
                updateSceneSlot(findSceneIndex(sceneId), sceneId, "");
            } else {
                sendError(3, requestUri, "Cannot delete scene that does not exist");
            }
            sendSuccess(requestUri+" deleted");
            break;
        default:
            sendError(4, requestUri, "Scene method not supported");
            break;
    }
}

void LightServiceClass::scenesIdLightFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method)
{
    switch (method) {
        case HTTP_PUT: {
            String body = HTTP->arg("plain");
            DEBUG_PRINT("Body: ");
            DEBUG_PRINTLN(body);
            // XXX Do something with this information...
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(body);
            if (root.success()) {
                sendJson(generateTargetPutResponse(root, "/scenes/" + handler->getWildCard(1) + "/lightstates/" + handler->getWildCard(2) + "/"));
            }
            break;
        }
        default:
            sendError(4, requestUri, "Scene method not supported");
            break;
    }
}

void LightServiceClass::groupListingHandler()
{
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    getGroupJson(root);
    sendJson(root);
}

// returns true on failure
bool LightServiceClass::updateGroupSlot(int slot, String body)
{
    if (body == "") {
        return false;
    }
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(body);
    if (!root.success() || !validateGroupCreateBody(root)) {
        // throw error bad body
        sendError(2, "groups/" + (slot + 1), "Bad JSON body");
        return false;
    } 
    DEBUG_PRINT("updateGroupSlot:");
    DEBUG_PRINTLN(body);
    String fileName = GROUP_FILE_TEMPLATE;
    fileName.replace("%d", String(slot));
    if (pLightScenes[slot]) {
        delete pLightScenes[slot];
        pLightScenes[slot] = nullptr;
        if (SPIFFS.exists(fileName)){
            SPIFFS.remove(fileName);   
        }
    }
    DEBUG_PRINT("Updating ");
    DEBUG_PRINTLN(fileName);
    pLightScenes[slot] = new LightGroup(root);

    jsonBuffer.clear();
    //DynamicJsonBuffer jsonBuffer2(capacity);
    JsonObject& save_root = jsonBuffer.createObject();
    pLightScenes[slot]->fillJson(save_root);
    saveToFile(fileName, save_root);
    return true;
}

void LightServiceClass::groupCreationHandler()
{
    // handle group creation
    // find first available group slot
    int availableSlot = -1;
    for (int i = 0; i < 16; i++) {  // TEST start at 1 instead of 0
        if (!pLightGroups[i]) {   
            availableSlot = i;
            break;
        }
    }
    if (availableSlot == -1) {
        // throw error no new groups allowed
        sendError(301, "groups", "Groups table full");
        return;
    }
    if (updateGroupSlot(availableSlot, HTTP->arg("plain"))) {
        String slot = "";
        slot += (availableSlot + 1);
        sendSuccess("id", slot);
    }
}

void LightServiceClass::groupsFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method)
{
    switch (method) {
        case HTTP_GET:
            groupListingHandler();
            break;
        case HTTP_POST:
            groupCreationHandler();
            break;
        default:
            sendError(4, requestUri, "Group method not supported");
            break;
    }
}

void LightServiceClass::groupsIdFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method)
{
    String groupNumText = handler->getWildCard(1);
    int groupNum = atoi(groupNumText.c_str()) - 1;
    if ((groupNum == -1 && groupNumText != "0") || groupNum >= 16 || (groupNum >= 0 && !pLightGroups[groupNum])) {
        // error, invalid group number
        sendError(3, requestUri, "Invalid group number");
        return;
    }

    switch (method) {
        case HTTP_GET: {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.createObject();
            if (groupNum != -1) {
                pLightScenes[groupNum]->fillJson(root);
                sendJson(root);
            } else {
                root["name"] = "0";
                JsonArray& lightsArray = root.createNestedArray("lights");
                for (int i = 0; i < getLightsAvailable(); i++) {
                    if (!pLightHandlers[i]) {
                        continue;
                    }
                    // add light to list
                    String lightNum = "";
                    lightNum += (i + 1);
                    lightsArray.add(lightNum);
                }
                sendJson(root);
            }
            break;
        }
        case HTTP_PUT: {
            // validate body, delete old group, create new group
            updateGroupSlot(groupNum, HTTP->arg("plain"));
            sendUpdated();
            break;
        }
        case HTTP_DELETE: {
            updateGroupSlot(groupNum, "");
            sendSuccess(requestUri+" deleted");
            break;
        }
        default: {
            sendError(4, requestUri, "Group method not supported");
            break;
        }
    }
}

bool LightServiceClass::parseHueLightInfo(LightInfo currentInfo, JsonObject& root, LightInfo *newInfo)
{
    *newInfo = currentInfo;
    if (root.containsKey("on")) {
        newInfo->on = root["on"];
    }
    // pull brightness
    if (root.containsKey("bri")) {
        newInfo->brightness = root["bri"];
    }
    // pull effect
    if (root.containsKey("effect")) {
        String effect = root["effect"];
        if (strcmp(effect.c_str(), "colorloop") == 0) {
            newInfo->effect = EFFECT_COLORLOOP;
        } else {
            newInfo->effect = EFFECT_NONE;
        }
    }
    // pull alert
    if (root.containsKey("alert")) {
        String alert = root["alert"];
        if (strcmp(alert.c_str(), "select") == 0) {
            newInfo->alert = ALERT_SELECT;
        } else if (strcmp(alert.c_str(), "lselect") == 0) {
            newInfo->alert = ALERT_LSELECT;
        } else {
            newInfo->alert = ALERT_NONE;
        }
    }
    // pull transitiontime
    if (root.containsKey("transitiontime")) {
        newInfo->transitionTime = root["transitiontime"];
    }
    if (root.containsKey("xy")) {
        JsonArray& xyState = root["xy"];
        if (xyState.size() != 2) {
            sendError(5, "/api/api/lights/?/state", "xy color coordinates incomplete");
            return false;
        }
        float x = xyState[0];
        float y = xyState[1];
        hsvcolor hsb = getXYtoRGB(x, y, newInfo->brightness);
        newInfo->hue = getHue(hsb);
        newInfo->saturation = getSaturation(hsb);
    } else if (root.containsKey("ct")) {
        int mirek = root["ct"];
        if (mirek > 500 || mirek < 153) {
            sendError(7, "/api/api/lights/?/state", "Invalid value for color temperature");
            return false;
        }
        hsvcolor hsb = getMirektoRGB(mirek);
        newInfo->hue = getHue(hsb);
        newInfo->saturation = getSaturation(hsb);
    } else {
        if (root.containsKey("hue")) {
            newInfo->hue = root["hue"];
        }
        if (root.containsKey("sat")) {
            newInfo->saturation = root["sat"];
        }
    }
    return true;
}

void LightServiceClass::applyConfigToLightMask(unsigned int lights)
{
    String body = HTTP->arg("plain");
    DEBUG_PRINT("applyConfigToLightMask:");
    DEBUG_PRINTLN(body);
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(body);
    if (root.success()) {
        for (int i = 0; i < getLightsAvailable(); i++) {
            LightHandler *handler = getLightHandler(i);
            LightInfo currentInfo = handler->getInfo(i);
            LightInfo newInfo;
            if (parseHueLightInfo(currentInfo, root, &newInfo)) {
                handler->handleQuery(i, newInfo, root);
            }
        }
        // As per the spec, the response can be "Updated." for memory-constrained devices
        sendUpdated();
    } else if (body != "") {
        // unparseable json
        sendError(2, "groups/0/action", "Bad JSON body in request");
    }
}

void LightServiceClass::groupsIdActionFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method)
{
    if (method != HTTP_PUT) {
        // error, only PUT allowed
        sendError(4, requestUri, "Only PUT supported for groups/*/action");
        return;
    }
    String groupNumText = handler->getWildCard(1);
    int groupNum = atoi(groupNumText.c_str()) - 1;
    if ((groupNum == -1 && groupNumText != "0") || groupNum >= 16 || (groupNum >= 0 && !pLightGroups[groupNum])) {
        // error, invalid group number
        sendError(3, requestUri, "Invalid group number");
        return;
    }
    // parse input as if for all lights
    unsigned int lightMask;
    if (groupNum == -1) {
        lightMask == 0xFFFF;
    } else {
        lightMask = pLightGroups[groupNum]->getLightMask();
    }
    // apply to group
    applyConfigToLightMask(lightMask);
}

void LightServiceClass::lightsFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method)
{
    switch (method) {
        case HTTP_GET: {
            // dump existing lights
            DynamicJsonBuffer jsonBuffer;
            JsonObject& lights = jsonBuffer.createObject();
            addLightsJson(lights);
            sendJson(lights);
            break;
        }
        case HTTP_POST:
            // "start" a "search" for "new" lights
            sendSuccess("/lights", "Searching for new devices");
            break;
        default:
            sendError(4, requestUri, "Light method not supported");
            break;
    }
}

void LightServiceClass::addSingleLightJson(JsonObject& root, int numberOfTheLight, LightHandler* lightHandler)
{
    if (!lightHandler) {
        return;
    }
    String lightNumber = (String) (numberOfTheLight + 1);
    String lightName = lightHandler->getFriendlyName(numberOfTheLight);
    root["manufacturername"] = "OpenSource";  // type of lamp (all "Extended colour light" for now)
    root["modelid"] = "LST001";  // the model number
    root["name"] = lightName;  // the name as set through the web UI or app
    JsonObject& state = root.createNestedObject("state");
    LightInfo info = lightHandler->getInfo(numberOfTheLight);
    state["on"] = info.on;
    state["hue"] = info.hue;  // hs mode: the hue (expressed in ~deg*182.04)
    state["bri"] = info.brightness;  // brightness between 0-254 (NB 0 is not off!)
    state["sat"] = info.saturation; // hs mode: saturation between 0-254
    JsonArray& xystate = state.createNestedArray("xy");
    xystate.add(0.0);
    xystate.add(0.0);
    state["ct"] = 500;  // ct mode: color temp (expressed in mireds range 154-500)
    state["alert"] = "none";  // 'select' flash the lamp once, 'lselect' repeat flash for 30s
    state["effect"] = info.effect == EFFECT_COLORLOOP ? "colorloop" : "none";
    state["colormode"] = "hs";  // the current color mode
    state["reachable"] = true;  // lamp can be seen by the hub
    state["swversion"] = "0.1";  
    if (info.bulbType == BulbType::DIMMABLE_LIGHT) {
        state["type"] = "Dimmable light";
    } else {
        state["type"] = "Extended color light";
    }
    state["uniqueid"] = lightNumber;
}

void LightServiceClass::lightsIdFn(WcFnRequestHandler *whandler, String requestUri, HTTPMethod method)
{
    int numberOfTheLight = atoi(whandler->getWildCard(1).c_str()) - 1;
    LightHandler *handler = getLightHandler(numberOfTheLight);
    switch (method) {
        case HTTP_GET: {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.createObject();
            addSingleLightJson(root, numberOfTheLight, handler);
            sendJson(root);
            break;
        }
        case HTTP_PUT:
            // XXX do something here
            sendUpdated();
            break;
        default:
            sendError(4, requestUri, "Light method not supported");
            break;
    }
}

void LightServiceClass::lightsIdStateFn(WcFnRequestHandler *whandler, String requestUri, HTTPMethod method)
{
    int numberOfTheLight = max(0, atoi(whandler->getWildCard(1).c_str()) - 1);
    LightHandler *handler = getLightHandler(numberOfTheLight);
    if (!handler) {
        char buff[100] = {};
        sprintf(buff, "Requested light not available for number %d", numberOfTheLight);
        sendError(3, requestUri, buff);
        return;
    }
    switch (method) {
        case HTTP_POST:
        case HTTP_PUT: {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& parsedRoot = jsonBuffer.parseObject(HTTP->arg("plain"));
            if (!parsedRoot.success()) {
                // unparseable json
                sendError(2, requestUri, "Bad JSON body in request");
                return;       
            }
            LightInfo currentInfo = handler->getInfo(numberOfTheLight);
            LightInfo newInfo;
            if (!parseHueLightInfo(currentInfo, parsedRoot, &newInfo)) {
                return;
            }
            handler->handleQuery(numberOfTheLight, newInfo, parsedRoot);
            sendJson(generateTargetPutResponse(parsedRoot, "/lights/" + whandler->getWildCard(1) + "/state/"));
            break;
        }
        default:
            sendError(4, requestUri, "Light method not supported");
            break;
    }
}

void LightServiceClass::lightsNewFn(WcFnRequestHandler *handler, String requestUri, HTTPMethod method)
{
    // dump empty object
    DynamicJsonBuffer jsonBuffer;
    JsonObject& lights = jsonBuffer.createObject();
    sendJson(lights);
}


// ==============================================================================================================
// Color Conversion
// ==============================================================================================================
// TODO: Consider switching to something along the lines of
// https://github.com/patdie421/mea-edomus/blob/master/src/philipshue_color.c
// and/ or https://github.com/kayno/arduinolifx/blob/master/color.h
// for color coversions instead
// ==============================================================================================================

// Based on http://stackoverflow.com/questions/22564187/rgb-to-philips-hue-hsb
// The code is based on this brilliant note: https://github.com/PhilipsHue/PhilipsHueSDK-iOS-OSX/commit/f41091cf671e13fe8c32fcced12604cd31cceaf3

rgbcolor LightServiceClass::getXYtoRGB(float x, float y, int brightness_raw)
{
  float Y = brightness_raw / 250.0f;

  float z = 1.0f - x - y;
  float X = (Y / y) * x;
  float Z = (Y / y) * z;

  // sRGB D65 conversion
  // See https://en.wikipedia.org/wiki/SRGB
  float r =  X * 3.2406f - Y * 1.5372f - Z * 0.4986f;
  float g = -X * 0.9689f + Y * 1.8758f + Z * 0.0415f;
  float b =  X * 0.0557f - Y * 0.2040f + Z * 1.0570f;

  if (r > b && r > g && r > 1.0f) {
    // red is too big
    g = g / r;
    b = b / r;
    r = 1.0f;
  }
  else if (g > b && g > r && g > 1.0f) {
    // green is too big
    r = r / g;
    b = b / g;
    g = 1.0f;
  }
  else if (b > r && b > g && b > 1.0f) {
    // blue is too big
    r = r / b;
    g = g / b;
    b = 1.0f;
  }

  // Apply gamma correction
  r = r <= 0.0031308f ? 12.92f * r : (1.0f + 0.055f) * pow(r, (1.0f / 2.4f)) - 0.055f;
  g = g <= 0.0031308f ? 12.92f * g : (1.0f + 0.055f) * pow(g, (1.0f / 2.4f)) - 0.055f;
  b = b <= 0.0031308f ? 12.92f * b : (1.0f + 0.055f) * pow(b, (1.0f / 2.4f)) - 0.055f;

  if (r > b && r > g) {
    // red is biggest
    if (r > 1.0f) {
      g = g / r;
      b = b / r;
      r = 1.0f;
    }
  }
  else if (g > b && g > r) {
    // green is biggest
    if (g > 1.0f) {
      r = r / g;
      b = b / g;
      g = 1.0f;
    }
  }
  else if (b > r && b > g) {
    // blue is biggest
    if (b > 1.0f) {
      r = r / b;
      g = g / b;
      b = 1.0f;
    }
  }

  r = r < 0 ? 0 : r;
  g = g < 0 ? 0 : g;
  b = b < 0 ? 0 : b;

  return rgbcolor(r * COLOR_SATURATION,
                  g * COLOR_SATURATION,
                  b * COLOR_SATURATION);
}

int LightServiceClass::getHue(hsvcolor hsb)
{
  return hsb.h * 360 * 182.04;
}

int LightServiceClass::getSaturation(hsvcolor hsb)
{
  return hsb.s * COLOR_SATURATION;
}

rgbcolor LightServiceClass::getMirektoRGB(int mirek)
{
  int hectemp = 10000 / mirek;
  int r, g, b;
  if (hectemp <= 66) {
    r = COLOR_SATURATION;
    g = 99.4708025861 * log(hectemp) - 161.1195681661;
    b = hectemp <= 19 ? 0 : (138.5177312231 * log(hectemp - 10) - 305.0447927307);
  } else {
    r = 329.698727446 * pow(hectemp - 60, -0.1332047592);
    g = 288.1221695283 * pow(hectemp - 60, -0.0755148492);
    b = COLOR_SATURATION;
  }
  r = r > COLOR_SATURATION ? COLOR_SATURATION : r;
  g = g > COLOR_SATURATION ? COLOR_SATURATION : g;
  b = b > COLOR_SATURATION ? COLOR_SATURATION : b;
  return rgbcolor(r, g, b);
}

// check the file system and restore group slots from them
void LightServiceClass::initializeGroupSlots()
{
    DEBUG_PRINTLN("initializeGroupSlots()");
    for (int i = 0; i < 16; i++) {
        String fileName = GROUP_FILE_TEMPLATE;
        fileName.replace("%d",String(i));
        //DEBUG_PRINT("Testing for ");
        //DEBUG_PRINTLN(fileName);
        if (SPIFFS.exists(fileName)) {   
            // read the file into the groupslot position
            DEBUG_PRINT("Open group file:");
            DEBUG_PRINTLN(fileName);
            File f  = SPIFFS.open(fileName,"r");
            if (f) {
                DynamicJsonBuffer jsonBuffer;
                // Parse the root object
                JsonObject &root = jsonBuffer.parseObject(f);
                if (!root.success()) {
                    DEBUG_PRINT("Failed to read group file:");
                    DEBUG_PRINTLN(fileName);
                } else {
                    DEBUG_PRINT("Loading ");
                    DEBUG_PRINTLN(fileName);
                    pLightGroups[i] = new LightGroup(root);
                }
                f.close();
            } else {
                DEBUG_PRINTLN("Can't open");
            }
        } else {
          //DEBUG_PRINTLN("Not found");
        }
    }
}

// check the file system and restore group slots from them
void LightServiceClass::initializeSceneSlots()
{
    DEBUG_PRINTLN("initializeSceneSlots()");
    for (int i = 0; i < 16; i++) {
        String fileName = SCENE_FILE_TEMPLATE;
        fileName.replace("%d",String(i));
        //DEBUG_PRINT("Testing for ");DEBUG_PRINTLN(fileName);
        if (SPIFFS.exists(fileName)) {   
            // read the file into the groupslot position
            DEBUG_PRINT("Open scene file:");
            DEBUG_PRINTLN(fileName);
            File f  = SPIFFS.open(fileName,"r");
            if (f) {
                DynamicJsonBuffer jsonBuffer;
                // Parse the root object
                JsonObject &root = jsonBuffer.parseObject(f);
                if (!root.success()) {
                    DEBUG_PRINT("Failed to read scene file:");
                    DEBUG_PRINTLN(fileName);
                } else {
                    DEBUG_PRINT("Loading ");
                    DEBUG_PRINTLN(fileName);
                    pLightScenes[i] = new LightGroup(root);
                    pLightScenes[i]->id = String(i,DEC);
                }
                f.close();
            } else {
                DEBUG_PRINTLN("Can't open");
            }
        } else {
            //DEBUG_PRINTLN("Not found");
        }  
    }
}

String methodToString(int method) {
  switch (method) {
    case HTTP_POST: return "POST";
    case HTTP_GET: return "GET";
    case HTTP_PUT: return "PUT";
    case HTTP_PATCH: return "PATCH";
    case HTTP_DELETE: return "DELETE";
    case HTTP_OPTIONS: return "OPTIONS";
    default: return "unknown";
  }
}

