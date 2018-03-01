/**************************************************************

This file is a part of
https://github.com/atiderko/esp8266-ansulta-alexa
Copyright (c) 2018 Alexander Tiderko

Licensed under MIT license

Configuration class to setup WiFi settings using
WiFiManager [https://github.com/tzapu/WiFiManager].
It is also possible to reset the configuration by double press
on reset. The idea is based on
https://github.com/datacute/DoubleResetDetector

**************************************************************/
#include "config.h"
#include "debug.h"
#include "ArduinoJSON.h"

static Config* reference = NULL;

//callback notifying us of the need to save config
void saveConfigCallback () {
    if (reference != NULL) {
      DEBUG_PRINTLN("Should save config");
      reference->should_save_config();
    }
}


Config::Config()
{
    device_name = HUE_DEVICE_NAME;
    pShouldSaveConfig = false;
    pAnsultaAddressA = 0x00;
    pAnsultaAddressB = 0x00;
}

Config::~Config()
{
    reference = NULL;
}

void Config::setup()
{
    bool remove_cfg = false;
    if (has_flag(RESET_UTC_ADDRESS, RESET_FLAG)) {
      //
      DEBUG_PRINTLN("RESET detected, remove configuration");
      remove_cfg = true;
    }
    set_flag(RESET_UTC_ADDRESS, RESET_FLAG);
    reference = this;
    //read configuration from FS json
    DEBUG_PRINTLN("mounting FS...");
    if (SPIFFS.begin()) {
        DEBUG_PRINTLN("mounted file system");
        if (remove_cfg) {
            if (SPIFFS.exists(CONFIG_FILE)) {
                DEBUG_PRINTLN("remove config file");
                SPIFFS.remove(CONFIG_FILE);
            }
        }
        if (SPIFFS.exists(CONFIG_FILE)) {
            //file exists, reading and loading
            DEBUG_PRINTLN("reading config file");
            File configFile = SPIFFS.open(CONFIG_FILE, "r");
            if (configFile) {
                DEBUG_PRINTLN("opened config file");
                size_t size = configFile.size();
                // Allocate a buffer to store contents of the file.
                std::unique_ptr<char[]> buf(new char[size]);

                configFile.readBytes(buf.get(), size);
                DynamicJsonBuffer jsonBuffer;
                JsonObject& json = jsonBuffer.parseObject(buf.get());
                json.printTo(Serial);
                if (json.success()) {
                    DEBUG_PRINTLN("\nparsed json");
                    device_name = json["device_name"].as<String>();
                    pAnsultaAddressA = json["ansulta_address_a"].as<byte>();
                    pAnsultaAddressB = json["ansulta_address_b"].as<byte>();
                    DEBUG_PRINT("Readed ansulta address, A:");
                    DEBUG_PRINT(pAnsultaAddressA);
                    DEBUG_PRINT(", B:");
                    DEBUG_PRINTLN(pAnsultaAddressA);
                } else {
                    DEBUG_PRINTLN("failed to load json config");
                }
            }
        }
    } else {
        DEBUG_PRINTLN("failed to mount FS");
    }
    WiFiManager wifiManager;
    wifiManager.setCustomHeadElement("<meta charset=\"UTF-8\">");
    if (remove_cfg) {
        wifiManager.resetSettings();
    }
    WiFiManagerParameter custom_ansulta_name("device_name", HUE_DEVICE_NAME, HUE_DEVICE_NAME, 80);
    wifiManager.addParameter(&custom_ansulta_name);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setConnectTimeout(20);
    if (!wifiManager.autoConnect(ANSULTA_AP, AP_PASSWORD)) {
        DEBUG_PRINTLN("failed to connect and hit timeout");
        set_flag(RESET_UTC_ADDRESS, RESET_FLAG_CLEAR);
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
    }
    set_flag(RESET_UTC_ADDRESS, RESET_FLAG_CLEAR);
    //if you get here you have connected to the WiFi
    DEBUG_PRINTLN("connected...yeey :)");
    device_name = custom_ansulta_name.getValue();

    if (pShouldSaveConfig) {
      p_save_config();
    }

    DEBUG_PRINT("local ip: ");
    DEBUG_PRINTLN(WiFi.localIP());
}

void Config::loop()
{
    
}

bool Config::is_connected()
{
  return (WiFi.status() == WL_CONNECTED);
}

void Config::should_save_config()
{
    pShouldSaveConfig = true;
}

void Config::save_ansulta_address(byte address_a, byte address_b)
{
    pAnsultaAddressA = address_a;
    pAnsultaAddressB = address_b;
    p_save_config();
}

byte Config::get_ansulta_address_a()
{
    return pAnsultaAddressA;
}

byte Config::get_ansulta_address_b()
{
    return pAnsultaAddressB;
}

void Config::p_save_config()
{
    //save the custom parameters to FS
    DEBUG_PRINTLN("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["device_name"] = device_name;
    json["ansulta_address_a"] = pAnsultaAddressA;
    json["ansulta_address_b"] = pAnsultaAddressB;
    File configFile = SPIFFS.open(CONFIG_FILE, "w");
    if (!configFile) {
        DEBUG_PRINTLN("failed to open config file for writing");
    }
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
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
