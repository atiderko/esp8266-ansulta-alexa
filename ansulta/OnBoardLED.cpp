/**************************************************************

This file is a part of
https://github.com/atiderko/esp8266-ansulta-alexa
Copyright (c) 2018 Alexander Tiderko

Licensed under MIT license

Controls the on board LED based on the current state.

**************************************************************/
#include "OnBoardLED.h"

OnBoardLED::OnBoardLED()
{
  p_led_state = LOW;
  p_previous_ms = 0;
  p_con_state = STARTING;
}

OnBoardLED::~OnBoardLED()
{
  
}

void OnBoardLED::init()
{
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, p_led_state);
}

void OnBoardLED::update()
{
  if (p_blink_count == 0) {
    // we are not in blink state -> set LED depend on current state
    int new_state = p_led_state;
    switch(p_con_state) {
      case STARTING:
        new_state = LOW;
        break;
      case OK:
        // new_state = p_get_state(10, 60000);
        new_state = HIGH;
        break;
      case NOT_CONNECTED:
        new_state = p_get_state(200, 200);
        break;
      case WIFI_CONNECTING:
        new_state = p_get_state(200, 2000);
        break;
      case ANSULTA_SEARCHING:
        new_state = p_get_state(2000, 4000);
        break;
    }
    if (new_state != p_led_state) {
      p_led_state = new_state;
      digitalWrite(LED_BUILTIN, p_led_state);
    }
  } else {
    // we are in blink state to visualize an action
    int current_ms = millis();
    if (current_ms - p_blink_previous_ms >= p_blink_interval) {
      if (p_led_state == HIGH) {
        digitalWrite(LED_BUILTIN, LOW);
        p_led_state = LOW;
      } else {
        digitalWrite(LED_BUILTIN, HIGH);
        p_led_state = HIGH;
        p_blink_count--;
      }
      p_blink_previous_ms = current_ms;
    }
  }
}

void OnBoardLED::set_connection_state(int state)
{
  p_con_state = state;
}

void OnBoardLED::blink(int count, int interval)
{
  p_blink_interval = interval;
  p_blink_count = count;
  if (p_led_state == LOW) {
    digitalWrite(LED_BUILTIN, HIGH);
    p_led_state = HIGH;
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    p_led_state = LOW;
  }
  p_blink_previous_ms = millis();
}

int OnBoardLED::p_get_state(unsigned long on_interval, unsigned long off_interval)
{
  int result = p_led_state;
  unsigned long current_ms = millis();
  unsigned long interval = on_interval;
  if (p_led_state == HIGH) {
    interval = off_interval;
  }
  if(current_ms - p_previous_ms >= interval) {
    p_previous_ms = current_ms;
    if (result == LOW)
      result = HIGH;  // Note that this switches the LED *off*
    else
      result = LOW;   // Note that this switches the LED *on*
  }
  return result;
}


