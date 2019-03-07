#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <functional>
#include "Ansulta.h"
#include "OnBoardLED.h"
#include "debug.h"
#include "config.h"
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()
#include "HueTypes.h"
#include "HueLightService.h"
#include "motion_detector.h"


Config cfg;
OnBoardLED led;
Ansulta ansulta;
MotionDetector motion;
int motion_state = 0;
bool saved_ansulta_address = false;
hue::LightServiceClass lightService(1);

// Handler used by LightServiceClass to switch the ansulta lights
class AnsultaHandler : public hue::LightHandler {
  private:
    hue::LightInfo _info;
  public:
    AnsultaHandler() {
        _info.bulbType =  hue::BulbType::DIMMABLE_LIGHT;
    }
    String getFriendlyName(int lightNumber) const {
        return cfg.device_name;  // defined in config.h
    }
    void handleQuery(int lightNumber, hue::LightInfo newInfo, JsonObject& raw) {
        if (newInfo.on) {
            DEBUG_PRINT("turn on " + this->getFriendlyName(lightNumber));
            if (!_info.on || _info.brightness > newInfo.brightness) {
                led.blink(2, 300);
                DEBUG_PRINTLN(" to 50%");
                ansulta.light_ON_50(50);
                newInfo.brightness = 127;
            } else {
                led.blink(3, 300);
                DEBUG_PRINTLN(" to 100%");
                ansulta.light_ON_100(50);
                newInfo.brightness = 254;
            }
        } else {
            // switch off
            DEBUG_PRINTLN("turn off " + this->getFriendlyName(lightNumber));
            led.blink(1, 300);
            ansulta.light_OFF(50);
        }
        _info = newInfo;
    }
    hue::LightInfo getInfo(int lightNumber) {
        _info.on = ansulta.get_state() != ansulta.OFF;
        if (ansulta.get_state() == ansulta.OFF) {
            _info.brightness = 0;
        } else if (ansulta.get_state() == ansulta.ON_50) {
            _info.brightness = 127;
        } else if (ansulta.get_state() == ansulta.ON_100) {
            _info.brightness = 254;
        }
        return _info;
    }
};

// defines used to set NTP date
#define TZ              1       // (utc+) TZ in hours
#define DST_MN          0      // use 60mn for summer time in some countries
#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)

// callback to set the NTP date in LightService
void time_is_set(void)
{
  lightService.ntp_available(true);
}


void setup()
{
    Serial.begin(115200);
    // init blue LED on board
    led.init();
    cfg.setup();
    saved_ansulta_address = ansulta.set_address(cfg.get_ansulta_address_a(), cfg.get_ansulta_address_b());
    lightService.begin();
    settimeofday_cb(time_is_set);
    // Sync our clock to NTP
    configTime(TZ_SEC, DST_SEC, "pool.ntp.org");
    ansulta.init();
    DEBUG_PRINTLN("Adding ansulta light switch");
    AnsultaHandler* ansulta_handler = new AnsultaHandler();
    lightService.setLightHandler(0, *ansulta_handler);
    motion.init(ansulta, cfg.motion_timeout, cfg.max_photo_intensity);
    ansulta.add_handler(&motion);
}
 
void loop()
{
    led.set_connection_state(led.NOT_CONNECTED);
    if (cfg.is_connected()) {
        lightService.update();
        delay(10);
        if (ansulta.valid_address()) {
           if (!saved_ansulta_address) {
              DEBUG_PRINT("ANSULTA ADDR: A");
              DEBUG_PRINT(ansulta.get_address_a());
              DEBUG_PRINT(",B:");
              DEBUG_PRINTLN(ansulta.get_address_b());
              cfg.save_ansulta_address(ansulta.get_address_a(), ansulta.get_address_b());
              saved_ansulta_address = true;
           }
           led.set_connection_state(led.OK);
        } else {
            led.set_connection_state(led.ANSULTA_SEARCHING);
//            led.set_connection_state(led.OK);  // THIS IS ONLY FOR TEST
        }
  	} else if (ansulta.valid_address()) {
        led.set_connection_state(led.WIFI_CONNECTING);
        delay(100);
  	}
    led.update();
    ansulta.serverLoop();
    delay(10);
    int mresult = motion.loop();
    if (motion_state != mresult) {
        DEBUG_PRINT("Motion state: ");
        DEBUG_PRINT(mresult);
        DEBUG_PRINTLN();
        if (mresult == 1) {
            led.blink(2, 250);
        } else if (mresult > 1) {
            led.blink(mresult, 1000 * mresult);
        }
        motion_state = mresult;
    }
}

