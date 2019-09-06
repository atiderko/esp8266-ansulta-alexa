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
bool saved_ansulta_address = false;
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
    //led.set_connection_state(led.NOT_CONNECTED);
    cfg.loop();
    if (cfg.is_connected()) {
        if (!initialized) {
            saved_ansulta_address = ansulta.set_address(cfg.get_ansulta_address_a(), cfg.get_ansulta_address_b());
            lightService.begin();
            settimeofday_cb(time_is_set);
            // Sync our clock to NTP
            configTime(TZ_SEC, DST_SEC, "pool.ntp.org");
            ansulta.init();
            DEBUG_PRINTLN("Adding ansulta light switch");
            AnsultaLightHandler* ansulta_handler = new AnsultaLightHandler(&ansulta, &cfg, &led);
            lightService.setLightHandler(0, *ansulta_handler);
            motion.init(ansulta, cfg.motion_timeout_sec * 1000, cfg.max_photo_intensity);
            ansulta.add_handler(&motion);
            initialized = true;
        }
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
//        delay(100);
  	}
    if (initialized) {
        led.update();
        ansulta.serverLoop();
    }
//    delay(10);
    if (cfg.has_motion()) {
        int mresult = motion.loop();
        cfg.monitor_photo_intensity = motion.current_photo_intensity();
        cfg.monitor_photo_intensity_smooth = motion.current_photo_intensity_smooth();
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
