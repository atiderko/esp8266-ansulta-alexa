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

class Ansulta {
public:
    static const int OFF = 0;
    static const int ON_50 = 1;
    static const int ON_100 = 2;

    Ansulta();
    ~Ansulta();
    void init();
    void serverLoop();
    bool valid_address();
    void light_ON_50(int count=50);
    void light_ON_100(int count=50);
    void light_OFF(int count=50);
    int get_state();
    bool set_address(byte addr_a, byte addr_b);
    byte get_address_a();
    byte get_address_b();

private:
    bool p_address_found;
    bool p_first_info;
    int p_count_c;
    byte delayA;
    unsigned int delayB;
    byte delayC;
    byte delayD;
    byte delayE;
    
    byte AddressByteA;
    byte AddressByteB;

    int p_count_repeats;
    int p_led_state;

    void ReadAddressBytes();
    byte ReadReg(byte addr);
    void SendStrobe(byte strobe, unsigned int delay_after=200);
    void SendCommand(byte AddressByteA, byte AddressByteB, byte Command, int count=10);
    void WriteReg(byte addr, byte value);
    void init_CC2500();
};
 
#endif
