/**************************************************************

This file is a part of
https://github.com/atiderko/esp8266-ansulta-alexa
Copyright (c) 2018 Alexander Tiderko

Licensed under MIT license

**************************************************************/
#include "config.h"
#include "debug.h"
#include "EEPROM.h"

static Config* reference = NULL;


Config::Config()
{
    device_name = HUE_DEVICE_NAME;
    motion_timeout_sec = MOTION_TIMEOUT;
    max_photo_intensity = MAX_PHOTO_INTENSITY; 
    pAnsultaAddressA = 0;
    pAnsultaAddressB = 0;
    monitor_photo_intensity = -1;
    monitor_photo_intensity_smooth = -1;
    monitor_light_state = 0;
    pCharDefaultTimeout = String(MOTION_TIMEOUT);
    pCharDefaultMPI = String(MAX_PHOTO_INTENSITY);
    pServer = new WebServer(80);
    pIotWebConf = new IotWebConf(ANSULTA_AP, &pDnsServer, pServer, AP_PASSWORD, CONFIG_VERSION);
}

Config::~Config()
{
    reference = NULL;
    delete pIotWebConf;
    delete pServer;
}

int getRSSIasQuality(int RSSI) {
    int quality = 0;
  
    if (RSSI <= -100) {
        quality = 0;
    } else if (RSSI >= -50) {
        quality = 100;
    } else {
        quality = 2 * (RSSI + 100);
    }
    return quality;
}

void Config::setup()
{
    if (has_flag(RESET_UTC_ADDRESS, RESET_FLAG)) {
        // double reset press detected
        // delete version tag of IotWebConf to force create new configuration
        DEBUG_PRINTLN("RESET detected, remove configuration");
        for (byte t = 0; t < IOTWEBCONF_CONFIG_VESION_LENGTH; t++){
            EEPROM.begin(IOTWEBCONF_CONFIG_START + IOTWEBCONF_CONFIG_VESION_LENGTH);
            EEPROM.write(IOTWEBCONF_CONFIG_START + t, 0);
            delay(200);
            EEPROM.commit();
            EEPROM.end();
        }
    }
    set_flag(RESET_UTC_ADDRESS, RESET_FLAG);
    delay(1500);
    set_flag(RESET_UTC_ADDRESS, RESET_FLAG_CLEAR);
    // -- Initializing the configuration.
    pIotWebConf->setStatusPin(LED_BUILTIN);
    // scan for WiFi networks and add a select box to the settings
    int numberOfNetworks = WiFi.scanNetworks();
    if (numberOfNetworks > 0) {
        pSSIDselectorString = "<div class=\"\"><label for=\"iwcWifiSsid\">WiFi SSID</label><div class=\"\" style=\"padding:0px;font-size:1em;width:100%;\"><select type=\"text\" id=\"iwcWifiSsid\" name=\"iwcWifiSsid\" maxlength=33 value=\"\"/>";
        for (int i = 0; i < numberOfNetworks; i++){
            pSSIDselectorString = pSSIDselectorString + "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + " (" + getRSSIasQuality(WiFi.RSSI(i)) + "%)</option>";
        }
        pSSIDselectorString = pSSIDselectorString + "</select></div></div>";
        pIotWebConf->getWifiSsidParameter()->label = NULL;
        pIotWebConf->getWifiSsidParameter()->customHtml = pSSIDselectorString.c_str();
    }
    pDeviceName = IotWebConfParameter("Device name", "deviceName", pDeviceNamePValue, STRING_LEN, "text", HUE_DEVICE_NAME, HUE_DEVICE_NAME, NULL, true);
    // add settings for motion detection
    pMotionSeparator = IotWebConfSeparator("Motion Detection");
    pMDselectorString = pMDselectorString + "<div class=\"\"><label for=\"mdEnabled\">Enabled: </label><select type=\"text\" id=\"mdEnabled\" name=\"mdEnabled\" maxlength=1 value=\"\"/><option value=\"" + p_has_motion + "\">" + p_has_motion + "</option><option value=\"" + !p_has_motion + "\">" + !p_has_motion + "</option></select></div>";
    pMotionEnabled = IotWebConfParameter(NULL, "mdEnabled", pMotionEnabledPValue, NUMBER_LEN, "number", "0,1", 0, pMDselectorString.c_str(), true);
    pMotionTimeout = IotWebConfParameter("Timeout (seconds until off)", "mdTimeout", pMotionTimeoutPValue, NUMBER_LEN, "number", pCharDefaultTimeout.c_str(), pCharDefaultTimeout.c_str(), NULL, true);
    pMotionMaxFotoIntensity = IotWebConfParameter("Max photo intensity", "mdMaxPhotoIntensity", pMotionMaxFotoIntensityPValue, NUMBER_LEN, "number", pCharDefaultMPI.c_str(), pCharDefaultMPI.c_str(), NULL, true);
    // add settings for ansulta
    pAnsultaSeparator = IotWebConfSeparator("Ansulta address (0: autodetected)");
    pIotParamAnsultaAddressA = IotWebConfParameter("AddressA", "ansulta_address_a", pIotParamAnsultaAddressAPValue, NUMBER_LEN, "number", "0..256", NULL, NULL, true);
    pIotParamAnsultaAddressB = IotWebConfParameter("AddressB", "ansulta_address_b", pIotParamAnsultaAddressBPValue, NUMBER_LEN, "number", "0..256", NULL, NULL, true);
    // clear errors by setting errorMessage to NULL
    pDeviceName.errorMessage = NULL;
    pMotionEnabled.errorMessage = NULL;
    pMotionTimeout.errorMessage = NULL;
    pMotionMaxFotoIntensity.errorMessage = NULL;
    pIotParamAnsultaAddressA.errorMessage = NULL;
    pIotParamAnsultaAddressB.errorMessage = NULL;
    pIotWebConf->addParameter(&pDeviceName);
    pIotWebConf->addParameter(&pMotionSeparator);
    pIotWebConf->addParameter(&pMotionEnabled);
    pIotWebConf->addParameter(&pMotionTimeout);
    pIotWebConf->addParameter(&pMotionMaxFotoIntensity);
    pIotWebConf->addParameter(&pAnsultaSeparator);
    pIotWebConf->addParameter(&pIotParamAnsultaAddressA);
    pIotWebConf->addParameter(&pIotParamAnsultaAddressB);
    pIotWebConf->setupUpdateServer(&pHttpUpdater);
    pIotWebConf->setConfigSavedCallback([&]() { config_saved(); });
    //pIotWebConf->setFormValidator(std::bind(&Config::form_validator, this));
    pIotWebConf->setFormValidator([&]() { return form_validator(); });
    pIotWebConf->init();
    p_convert_cfg_values();
    
    // -- Set up required URL handlers on the web server.
    pServer->on("/", [&]() { handle_root(); });
    pServer->on("/devconfig", [&]{ pIotWebConf->handleConfig(); });
    pServer->onNotFound([&](){ pIotWebConf->handleNotFound(); });
}

void Config::p_convert_cfg_values()
{
    device_name = strlen(pDeviceNamePValue) == 0 ? HUE_DEVICE_NAME : String(pDeviceNamePValue);
    p_has_motion = strlen(pMotionEnabledPValue) == 0 ? false : atoi(pMotionEnabledPValue);
    motion_timeout_sec = strlen(pMotionTimeoutPValue) == 0 ? MOTION_TIMEOUT : atoi(pMotionTimeoutPValue);
    max_photo_intensity = strlen(pMotionMaxFotoIntensityPValue) == 0 ? MAX_PHOTO_INTENSITY : atoi(pMotionMaxFotoIntensityPValue);
    pAnsultaAddressA = strlen(pIotParamAnsultaAddressAPValue) == 0 ? 0 : atoi(pIotParamAnsultaAddressAPValue);
    pAnsultaAddressB = strlen(pIotParamAnsultaAddressBPValue) == 0 ? 0 : atoi(pIotParamAnsultaAddressBPValue);
}

void Config::loop()
{
    pIotWebConf->doLoop();
}

void Config::handle_root()
{
    // -- Let IotWebConf test and handle captive portal requests.
    if (pIotWebConf->handleCaptivePortal())
    {
      // -- Captive portal request were already served.
      return;
    }
    // <meta name=\"viewport\" http-equiv=\"refresh\" content=\"3\" charset=\"UTF-8\"/>
    String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=yes\" charset=\"UTF-8\"/>";
    s += "<title>Ansulta controller</title></head><body><h1>Alexa ESP8266 Ansulta controller</h1>";
    s += "<p>AP defaults (address: <i>" + String(pIotWebConf->getThingName()) + ":80</i> user: <i>admin</i> pw: <i>" + String(AP_PASSWORD) + "</i>)</p>";
    s += "<ul>";
    s += "<li>Current SSID: ";
    if (is_connected()) {
        s += pIotWebConf->getWifiSsidParameter()->valueBuffer;
    } else if (strlen(pIotWebConf->getWifiSsidParameter()->valueBuffer) == 0) {
        s += "not configured";
    } else {
        s += pIotWebConf->getWifiSsidParameter()->valueBuffer;
        s += "not connected";
    }
    s += "</li><li>Device name: ";
    s += device_name;
    s += "</li><li>Motion Detection: ";
    if (p_has_motion) {
        s += "enabled";
        s += "<ul>";
        s += "<li>Timeout: " + String(motion_timeout_sec) + " sec</li>";
        s += "<li>Max photo intensity: " + String(max_photo_intensity) + "</li>";
        s += "</ul>";
    } else {
        s += "disabled";
    }
    s += "</li><li>Light state: ";
    if (monitor_light_state == 0) {
      s += "unknown";
    } else if (monitor_light_state == 1) {
      s += "OFF";
    } else if (monitor_light_state == 2) {
      s += "ON (50%)";
    } else if (monitor_light_state == 3) {
      s += "ON (100%)";
    }
    s += "</li><li>Ansulta address: ";
    if (pAnsultaAddressA == 0 && pAnsultaAddressB == 0) {
        s += "<b>Waiting for ansulta control signal!</b>";
    } else {
        s += String(pAnsultaAddressA) + ":" + String(pAnsultaAddressB);
    }
    s += "</li><li>Current photo intensity: ";
    s += (monitor_photo_intensity == -1) ? "---" : String(monitor_photo_intensity);
    s += " (smooth: ";
    s += (monitor_photo_intensity_smooth == -1) ? "---" : String(monitor_photo_intensity_smooth);
    s += ")</li></ul>";
    s += "Go to <a href='devconfig'>configure page</a> to change settings.";
    s += "</body></html>\n";
    pServer->send(200, "text/html", s);
}

void Config::config_saved()
{
    p_convert_cfg_values();
    DEBUG_PRINTLN("Configuration was updated, values updated!");
}

boolean Config::form_validator()
{
    DEBUG_PRINTLN("TODO: validate input fields!");
    pDeviceName.errorMessage = NULL;
    pMotionEnabled.errorMessage = NULL;
    pMotionTimeout.errorMessage = NULL;
    pMotionMaxFotoIntensity.errorMessage = NULL;
    pIotParamAnsultaAddressA.errorMessage = NULL;
    pIotParamAnsultaAddressB.errorMessage = NULL;
    return true;
}

bool Config::is_connected()
{
    return (pIotWebConf->getState() == IOTWEBCONF_STATE_ONLINE);
}

bool Config::has_motion()
{
    return p_has_motion;  
}

void Config::save_ansulta_address(byte address_a, byte address_b)
{
    pAnsultaAddressA = address_a;
    pAnsultaAddressB = address_b;
    itoa(address_a, pIotParamAnsultaAddressAPValue, 10);
    itoa(address_b, pIotParamAnsultaAddressBPValue, 10);
    pIotWebConf->configSave();
}

byte Config::get_ansulta_address_a()
{
    return pAnsultaAddressA;
}

byte Config::get_ansulta_address_b()
{
    return pAnsultaAddressB;
}

bool Config::has_flag(int address, uint32_t flag)
{
    uint32_t read_flag;
    ESP.rtcUserMemoryRead(address, &read_flag, sizeof(read_flag));
    return read_flag == flag;
}

void Config::set_flag(int address, uint32_t flag)
{
    ESP.rtcUserMemoryWrite(address, &flag, sizeof(flag));
}
