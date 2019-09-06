/**************************************************************

This file is a part of
https://github.com/atiderko/esp8266-ansulta-alexa
Copyright (c) 2018 Alexander Tiderko

Licensed under MIT license

Configuration class to setup WiFi settings using
IoTWebConf [https://github.com/prampec/IotWebConf].
It is also possible to reset the configuration by double press
on reset. The idea is based on
https://github.com/datacute/DoubleResetDetector

**************************************************************/
#ifndef CONFIG_H
#define CONFIG_H

#include <IotWebConf.h>
#define IOTWEBCONF_DEBUG_TO_SERIAL true

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "ansulta v2"
#define STRING_LEN 128
#define NUMBER_LEN 32
#define STATUS_PIN LED_BUILTIN

const char ANSULTA_AP[] = "AnsultaAP";
const char AP_PASSWORD[] = "defaultpw";
static char HUE_DEVICE_NAME[] = "Küchenlicht";

// use reset idea from https://github.com/datacute/DoubleResetDetector
#define RESET_UTC_ADDRESS 127
#define RESET_FLAG 0x12121212
#define RESET_FLAG_CLEAR 0x10000001

class Config {
public:
    static const int MOTION_TIMEOUT = 35; // seconds, 0 disables motion
    static const int MAX_PHOTO_INTENSITY = 125;

    String device_name;
    int motion_timeout_sec;
    int max_photo_intensity;
    int monitor_photo_intensity;
    int monitor_photo_intensity_smooth;
    Config();
    ~Config();
    void setup();
    void loop();
    void config_saved();
    void handle_root();
    boolean form_validator();
    bool is_connected();
    bool has_motion();
    void save_ansulta_address(byte address_a, byte address_b);
    byte get_ansulta_address_a();
    byte get_ansulta_address_b();

protected:
    byte pAnsultaAddressA;
    byte pAnsultaAddressB;
    bool p_has_motion;
    bool has_flag(int address, uint32_t flag);
    void set_flag(int address, uint32_t flag);
    void p_convert_cfg_values();
    // web configuration parameter
    DNSServer pDnsServer;
    HTTPUpdateServer pHttpUpdater;
    WebServer *pServer;
    IotWebConf *pIotWebConf;
    String pSSIDselectorString;
    String pMDselectorString;
    String pCharDefaultTimeout;
    String pCharDefaultMPI;
    char pDeviceNamePValue[STRING_LEN];
    char pIotParamAnsultaAddressAPValue[NUMBER_LEN];
    char pIotParamAnsultaAddressBPValue[NUMBER_LEN];
    char pMotionEnabledPValue[NUMBER_LEN];
    char pMotionTimeoutPValue[NUMBER_LEN];
    char pMotionMaxFotoIntensityPValue[NUMBER_LEN];
    IotWebConfParameter pDeviceName;
    IotWebConfSeparator pAnsultaSeparator;
    IotWebConfParameter pIotParamAnsultaAddressA;
    IotWebConfParameter pIotParamAnsultaAddressB;
    IotWebConfSeparator pMotionSeparator;
    IotWebConfParameter pMotionEnabled;
    IotWebConfParameter pMotionTimeout;
    IotWebConfParameter pMotionMaxFotoIntensity;
};

#endif
