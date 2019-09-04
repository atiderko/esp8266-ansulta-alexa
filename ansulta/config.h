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

IoTWebConf: https://github.com/prampec/IotWebConf

**************************************************************/
#ifndef CONFIG_H
#define CONFIG_H

#include <IotWebConf.h>
#define IOTWEBCONF_DEBUG_TO_SERIAL true

#include <FS.h>
#include <DNSServer.h>
//#include <WiFiManager.h>

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "ansulta v2"
#define CONFIG_FILE "/ansulta_config.json"
#define STRING_LEN 128
#define NUMBER_LEN 32
#define STATUS_PIN LED_BUILTIN
//#define ANSULTA_AP "AnsultaAP"
//#define AP_PASSWORD "ansulta"
// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char ANSULTA_AP[] = "AnsultaAP";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char AP_PASSWORD[] = "defaultpw";

static char HUE_DEVICE_NAME[] = "Küchenlicht";

// use reset idea from https://github.com/datacute/DoubleResetDetector
#define RESET_UTC_ADDRESS 127
#define RESET_FLAG 0x12121212
#define RESET_FLAG_CLEAR 0x10000001

class Config {
public:
    static const unsigned long MOTION_TIMEOUT = 0; // millis, 0 disables motion
    static const int MAX_PHOTO_INTENSITY = 100;

    String device_name;
    unsigned long motion_timeout;
    int max_photo_intensity;
    Config();
    ~Config();
    void setup();
    void loop();
    void handle_root();
    bool is_connected();
    bool has_motion();
    void should_save_config();
    void save_ansulta_address(byte address_a, byte address_b);
    byte get_ansulta_address_a();
    byte get_ansulta_address_b();

protected:
    //flag for saving data
    bool pShouldSaveConfig;
    byte pAnsultaAddressA;
    byte pAnsultaAddressB;
    bool p_has_motion;
    void p_save_config();
    bool has_flag(int address, uint32_t flag);
    void set_flag(int address, uint32_t flag);
    // WiFiManager wifiManager;
    DNSServer pDnsServer;
    WebServer *pServer;
    IotWebConf *pIotWebConf;
    String ssid_selector;
    IotWebConfParameter *ssidParam;
    char ssidValue[NUMBER_LEN];
};

#endif
