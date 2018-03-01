/**************************************************************

This file is a part of
https://github.com/atiderko/esp8266-ansulta-alexa
Copyright (c) 2018 Alexander Tiderko

Licensed under MIT license

Controls the on board LED based on the current state.

**************************************************************/
#ifndef ONBOARDLED_H
#define ONBOARDLED_H
#include <ESP8266WiFi.h>

class OnBoardLED {
public:
    static const int STARTING = 0;
    static const int OK = 1;
    static const int NOT_CONNECTED = 2;
    static const int WIFI_CONNECTING = 3;
    static const int ANSULTA_SEARCHING = 4;

    OnBoardLED();
    ~OnBoardLED();
    void init();
    void update();
    void set_connection_state(int state);
    /** Blink to visualize an action. The blink state interrupts the visualization of connection state. */
    void blink(int count, int interval);

protected:
    int p_led_state;
    unsigned long p_previous_ms;
    int p_con_state;
    int p_blink_count;
    int p_blink_interval;
    unsigned long p_blink_previous_ms;
  
    int p_get_state(unsigned long on_interval, unsigned long off_interval);
};
 
#endif
