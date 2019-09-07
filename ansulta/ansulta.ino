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
#include "AnsultaLightHandler.h"

Config cfg;
OnBoardLED led;
Ansulta ansulta;
MotionDetector motion;
int motion_state = 0;
hue::LightServiceClass lightService(1);
bool initialized = false;

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
}


void loop()
{
    led.set_connection_state(led.NOT_CONNECTED);
    cfg.loop();
    if (cfg.is_connected()) {
        if (!initialized) {
            lightService.begin(cfg.webserver());
            settimeofday_cb(time_is_set);
            // Sync our clock to NTP
            configTime(TZ_SEC, DST_SEC, "pool.ntp.org");
            ansulta.setup(cfg);
            DEBUG_PRINTLN("Adding ansulta light switch");
            AnsultaLightHandler* ansulta_handler = new AnsultaLightHandler(&ansulta, &cfg, &led);
            lightService.setLightHandler(0, *ansulta_handler);
            motion.setup(ansulta, cfg);
            ansulta.add_handler(&motion);
            initialized = true;
        }
        lightService.update();
        delay(10);
        if (ansulta.valid_address()) {
           led.set_connection_state(led.OK);
        } else {
            led.set_connection_state(led.ANSULTA_SEARCHING);
//            led.set_connection_state(led.OK);  // THIS IS ONLY FOR TEST
        }
  	} else if (initialized && ansulta.valid_address()) {
        led.set_connection_state(led.WIFI_CONNECTING);
  	}
    if (initialized) {
        led.update();
        ansulta.serverLoop();
        if (cfg.has_motion()) {
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
    }
}
