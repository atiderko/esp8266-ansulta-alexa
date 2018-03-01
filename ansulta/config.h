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
#ifndef CONFIG_H
#define CONFIG_H

#include <FS.h>
#include <DNSServer.h>
#include <WiFiManager.h>

#define CONFIG_FILE "/ansulta_config.json"
#define ANSULTA_AP "AnsultaAP"
#define AP_PASSWORD "ansulta"

static char HUE_DEVICE_NAME[] = "Küchenlicht";

// use reset idea from https://github.com/datacute/DoubleResetDetector
#define RESET_UTC_ADDRESS 127
#define RESET_FLAG 0x12121212
#define RESET_FLAG_CLEAR 0x10000001

class Config {
public:
    String device_name;
    Config();
    ~Config();
    void setup();
    void loop();
    bool is_connected();
    void should_save_config();
    void save_ansulta_address(byte address_a, byte address_b);
    byte get_ansulta_address_a();
    byte get_ansulta_address_b();

protected:
    //flag for saving data
    bool pShouldSaveConfig;
    byte pAnsultaAddressA;
    byte pAnsultaAddressB;
    void p_save_config();
    bool has_flag(int address, uint32_t flag);
    void set_flag(int address, uint32_t flag);
};

#endif

