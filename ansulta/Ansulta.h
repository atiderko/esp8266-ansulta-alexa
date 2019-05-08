/**************************************************************

This file is a part of
https://github.com/atiderko/esp8266-ansulta-alexa

Licensed under MIT license

This class control the IKEA Ansulta lights using
CC2500 2.4Ghz wireless controller.
The code is based on
https://github.com/NDBCK/Ansluta-Remote-Controller

 **************************************************************/
#ifndef ANSULTA_H
#define ANSULTA_H

#include "cc2500_REG.h"
#include "cc2500_VAL.h"
#include <SPI.h>
#include "debug.h"

#define MAX_LEARN_TRIES 10        // Tries to receive the code from ansulta remote
#define REPEATS         1         // Tries to receive the code from ansulta remote

#define CC2500_SIDLE    0x36      // Exit RX / TX
#define CC2500_STX      0x35      // Enable TX. If in RX state, only enable TX if CCA passes
#define CC2500_SFTX     0x3B      // Flush the TX FIFO buffer. Only issue SFTX in IDLE or TXFIFO_UNDERFLOW states
#define CC2500_SRES     0x30      // Reset chip
#define CC2500_FIFO     0x3F      // TX and RX FIFO
#define CC2500_SRX      0x34      // Enable RX. Perform calibration if enabled
#define CC2500_SFRX     0x3A      // Flush the RX FIFO buffer. Only issue SFRX in IDLE or RXFIFO_OVERFLOW states

#define Light_OFF       0x01      // Command to turn the light off
#define Light_ON_50     0x02      // Command to turn the light on 50%
#define Light_ON_100    0x03      // Command to turn the light on 100%
#define Light_PAIR      0xFF      // Command to pair a remote to the light

class AnsultaCallback {
  public:
    virtual void light_state_changed(int state, bool by_ansulta_ctrl);
};

class Ansulta {
public:
    static const byte OFF = 0x01;
    static const byte ON_50 = 0x02;
    static const byte ON_100 = 0x03;

    Ansulta();
    ~Ansulta();
    void init();
    void serverLoop();
    void add_handler(AnsultaCallback *handler);
    bool valid_address();
    void light_ON_50(int count=50, bool disable_motion_detection=false, int brightness=127);
    void light_ON_100(int count=50, bool disable_motion_detection=false, int brightness=254);
    void light_OFF(int count=50, bool disable_motion_detection=false, int brightness=1);
    int get_state();
    bool set_address(byte addr_a, byte addr_b);
    int get_brightness();
    byte get_address_a();
    byte get_address_b();

private:
    std::vector<AnsultaCallback *> p_ansulta_handler;
    bool p_address_found;
    bool p_first_info;
    int p_count_c;
    byte delayA;
    unsigned int delayB;
    byte delayC;
    byte delayD;
    byte delayE;
    int p_brightness;
    
    byte AddressByteA;
    byte AddressByteB;
    byte p_led_state;

    int p_count_repeats;
    unsigned long p_off_last_cmd_ms;

    void inform_handler(int state, bool by_ansulta_ctrl);
    void read_cmd();
    void ReadAddressBytes();
    byte ReadReg(byte addr);
    void SendStrobe(byte strobe, unsigned int delay_after=200);
    void SendCommand(byte AddressByteA, byte AddressByteB, byte Command, int count=10);
    void WriteReg(byte addr, byte value);
    void init_CC2500();
};
 
#endif
