/**************************************************************

This file is a part of
https://github.com/atiderko/esp8266-ansulta-alexa

Licensed under MIT license

This class handles the detected motions.

 **************************************************************/
#include "motion_detector.h"
#include "debug.h"
#include <sys/time.h>


MotionDetector::MotionDetector() {
    p_ansulta = NULL;
    p_max_photo_intensity = 100;
    p_timeout_default = 35000;
    p_timeout = p_timeout_default;
    p_md1_pin = D0;
    p_photo_pin = A0;
    p_light_state = Ansulta::OFF;
    p_count_disable = 0;
    p_count_detected = 0;
    p_motion_ts_deactivated = 0;
    p_light_ts_manual_off = 0;
    p_md1_ts_detection = 0;
    p_light_ts_manual_intervantion = 0;
    p_disable_duration = 0;
    p_photo_state_smooth = 25;
}

void MotionDetector::init(Ansulta& ansulta, unsigned long timeout, int max_photo_intensity) {
    p_ansulta = &ansulta;
    p_max_photo_intensity = max_photo_intensity;
    p_timeout_default = timeout;
    p_timeout = timeout;
    pinMode(p_md1_pin, INPUT);
}

int MotionDetector::loop() {
    if (p_ansulta == NULL) {
        return 0;
    }
    unsigned long current_time = msecs();
    if (current_time - p_motion_ts_deactivated < p_disable_duration) {
        // motion detection for 1h deactivated
        if (p_disable_duration > 21600000) {
            return 4;
        } else if (p_disable_duration > 3600000) {
            return 3;
        } else {
            return 2;
        }
    }
    if (current_time - p_light_ts_manual_intervantion < 5000) {
        // motion detection for 5 seconds because of last manual intervantion by ansulta control
        return 0;
    }
    // handle motion detection
    bool is_on = p_light_state != Ansulta::OFF;
    int md1_state = digitalRead(p_md1_pin);
    // motion detected
    if (md1_state == HIGH) {
        p_count_detected++;
        // DEBUG_PRINT("detected count: ");
        // DEBUG_PRINT(p_count_detected);
        // DEBUG_PRINTLN();
        p_md1_ts_detection = current_time;
        if (!is_on && p_count_detected > 1) {
          int photo_state = analogRead(p_photo_pin);
          p_photo_state_smooth = p_photo_state_smooth * 0.8 + photo_state * 0.2;
          DEBUG_PRINT("photo value: ");
          DEBUG_PRINT(photo_state);
          DEBUG_PRINT(", smooth: ");
          DEBUG_PRINT(p_photo_state_smooth);
          if (p_photo_state_smooth <= p_max_photo_intensity) {
              if (p_photo_state_smooth < (p_max_photo_intensity / 3)) {
                  DEBUG_PRINT(" > on 50%");
                  p_ansulta->light_ON_50();
              } else{
                  DEBUG_PRINT(" > on 100%");
                  p_ansulta->light_ON_100();
              }
          } else {
              DEBUG_PRINT(" > INGNORE");
          }
          DEBUG_PRINTLN();
        }
    } else {
        p_count_detected = 0;
        if (is_on && (p_md1_ts_detection != 0) && (current_time - p_md1_ts_detection) > p_timeout) {
            p_ansulta->light_OFF();
            // after 1h without detection set to default timout
            p_timeout = p_timeout_default;
        }
    }
    if (md1_state == HIGH && (p_count_detected % 10) == 1) {
        return 1;
    } else {
        return 0;
    }
}

void MotionDetector::light_state_changed(int state, bool by_ansulta_ctrl) {
    DEBUG_PRINT("Reported light state to motion detector: ");
    DEBUG_PRINT(state);
    DEBUG_PRINT(" ansulta ctrl: ");
    DEBUG_PRINT(by_ansulta_ctrl);
    DEBUG_PRINTLN();
    if (p_light_state == state) {
        DEBUG_PRINTLN("motion detection current in state, ignore!");
        return;
    }
    unsigned long current_ts = msecs();
    if (by_ansulta_ctrl) {
        if (state == Ansulta::OFF) {
            if (current_ts - p_light_ts_manual_off < 5000) {
                p_count_disable++;
                if (p_count_disable == 1) {
                    DEBUG_PRINTLN("2x OFF in 3 sec, disable motion detection for 1h");
                    p_disable_duration = 3600000;
                } else if (p_count_disable == 2) {
                    DEBUG_PRINTLN("3x OFF in 3 sec, disable motion detection for 6h");
                    p_disable_duration = 3600000 * 6;
                } else if (p_count_disable == 3) {
                    DEBUG_PRINTLN("4x OFF in 3 sec, disable motion detection for 24h");
                    p_disable_duration = 3600000 * 24;
                }
                p_motion_ts_deactivated = current_ts;
            } else if (p_motion_ts_deactivated > 0) {
                DEBUG_PRINTLN("OFF, motion detection activated");
                p_motion_ts_deactivated = 0;
                p_count_disable = 0;
                p_disable_duration = 0;
            }
            p_timeout = p_timeout_default;
            p_light_ts_manual_off = current_ts;
        } else {
            DEBUG_PRINTLN("ON by ctrl, increase timeout for motion detection to 1h");
            p_timeout = 3600000;
        }
        p_light_ts_manual_intervantion = current_ts;
    }
    p_light_state = state;
}

unsigned long MotionDetector::msecs() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    unsigned long ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    return ms;
}
