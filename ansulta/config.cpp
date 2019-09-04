/**************************************************************

This file is a part of
https://github.com/atiderko/esp8266-ansulta-alexa
Copyright (c) 2018 Alexander Tiderko

Licensed under MIT license

**************************************************************/
#include "config.h"
#include "debug.h"

static Config* reference = NULL;


Config::Config()
{
    device_name = HUE_DEVICE_NAME;
    motion_timeout = MOTION_TIMEOUT;
    max_photo_intensity = MAX_PHOTO_INTENSITY; 
    pAnsultaAddressA = 0x00;
    pAnsultaAddressB = 0x00;
    pServer = new WebServer(80);
    pIotWebConf = new IotWebConf(ANSULTA_AP, &pDnsServer, pServer, AP_PASSWORD, CONFIG_VERSION);
}

Config::~Config()
{
    reference = NULL;
    delete pIotWebConf;
    delete pServer;
    delete ssidParam;
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
            EEPROM.commit():
            EEPROM.end();
        }
    }
    set_flag(RESET_UTC_ADDRESS, RESET_FLAG);
    delay(500);
    set_flag(RESET_UTC_ADDRESS, RESET_FLAG_CLEAR);
    // -- Initializing the configuration.
    pIotWebConf->setStatusPin(STATUS_PIN);
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
    pDeviceName = IotWebConfParameter("Device name", "deviceName", device_name, 128, "text", HUE_DEVICE_NAME, HUE_DEVICE_NAME, NULL, true);
    // add settings for motion detection
    pMotionSeparator = IotWebConfSeparator("Motion Detection");
    pMotionEnabled = IotWebConfParameter(NULL, "mdEnabled", p_has_motion, NUMBER_LEN, "number", "0,1", 0, "<div class=\"\"><label for=\"mdEnabled\">Enabled</label><div class=\"\" style=\"padding:0px;font-size:1em;width:100%;\"><select type=\"text\" id=\"mdEnabled\" name=\"mdEnabled\" maxlength=1 value=\"\"/><option value=\"" + p_has_motion + "\">" + p_has_motion + "</option><option value=\"" + !p_has_motion + "\">" + !p_has_motion + "</option></select></div></div>", true);
    pMotionTimeout = IotWebConfParameter("Timeout", "mdTimeout", motion_timeout_sec, NUMBER_LEN, "number", MAX_PHOTO_INTENSITY, MAX_PHOTO_INTENSITY, "seconds until off", true);
    pMotionMaxFotoIntensity = IotWebConfParameter("Max photo intensity", "mdMaxPhotoIntensity", max_photo_intensity, NUMBER_LEN, "number", MAX_PHOTO_INTENSITY, MAX_PHOTO_INTENSITY, "smaller value turn on light on day", true);
    // add settings for ansulta
    pAnsultaSeparator = IotWebConfSeparator("Ansulta address (autodetect)");
    pIotParamAnsultaAddressA = IotWebConfParameter("AddressA", "ansulta_address_a", pAnsultaAddressA, NUMBER_LEN, "number", "0..256", NULL, "0: autodetect", true);
    pIotParamAnsultaAddressB = IotWebConfParameter("AddressB", "ansulta_address_b", pAnsultaAddressB, NUMBER_LEN, "number", "0..256", NULL, "0: autodetect", true);
    pIotWebConf->addParameter(&pDeviceName);
    pIotWebConf->addParameter(&pMotionSeparator);
    pIotWebConf->addParameter(&pMotionEnabled);
    pIotWebConf->addParameter(&pMotionTimeout);
    pIotWebConf->addParameter(&pMotionMaxFotoIntensity);
    pIotWebConf->addParameter(&pAnsultaSeparator);
    pIotWebConf->addParameter(&pIotParamAnsultaAddressA);
    pIotWebConf->addParameter(&pIotParamAnsultaAddressB);
    pIotWebConf->setupUpdateServer(&pHttpUpdater);
    pIotWebConf->init();
    
    // -- Set up required URL handlers on the web server.
    pServer->on("/", [&]() { handle_root(); });
    pServer->on("/config", [&]{ pIotWebConf->handleConfig(); });
    pServer->onNotFound([&](){ pIotWebConf->handleNotFound(); });
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
    String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
    s += "<title>Alexa ESP8266 Ansulta controller/title></head><body>Alexa ESP8266 Ansulta controller<br>";
    s += "Go to <a href='config'>configure page</a> to change settings.<br>";
    s += "Default AP password: " + AP_PASSWORD;
    if (pAnsultaAddressA == 0x00 || pAnsultaAddressA == 0x00) {
        s += "<b>Waiting for ansulta control signal!</b>.<br>";
    }
    s += "</body></html>\n";
    pServer->send(200, "text/html", s);
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
