/**************************************************************

This file is a part of
https://github.com/atiderko/esp8266-ansulta-alexa

Licensed under MIT license

This class control the IKEA Ansulta lights using
CC2500 2.4Ghz wireless controller.
The code is based on
https://github.com/NDBCK/Ansluta-Remote-Controller

 **************************************************************/

 #include "Ansulta.h"

Ansulta::Ansulta()
{
  p_address_found = false;
  p_first_info = false;
  p_count_c = 0;
  // delays are in microseconds
  delayA = 1;  //1++ 0-- No delay is also possible
  // delayB = 15000; // 10000-- 20000++ 15000++     //KRITISCH
  delayB = 2000;  // delay in SendStrobe while send command
  // delayC = 10;  //255++ 128++ 64--
  delayC = 255;  //255++ 128++ 64--  // delay after send command
  delayD = 0;  //200++ 128+++ 64+++ 32+++ 8++
  delayE = 200;

  AddressByteA = 0x00;
  AddressByteB = 0x00;
}

Ansulta::~Ansulta()
{
  
}

void Ansulta::init(){
  pinMode(SS, OUTPUT);
  DEBUG_PRINTLN("Ansulta: Debug mode");
  DEBUG_PRINT("Ansulta: Initialisation");
  SPI.begin();
  SPI.beginTransaction(SPISettings(6000000, MSBFIRST, SPI_MODE0));    //Faster SPI mode 6000000, maximal speed for the CC2500 without the need for extra delays
  digitalWrite(SS, HIGH);
  SendStrobe(CC2500_SRES); //0x30 SRES Reset chip.
  SendStrobe(CC2500_SRES); //0x30 SRES Reset chip.
  init_CC2500();
  //  SendStrobe(CC2500_SPWD); //Enter power down mode    -   Not used in the prototype
  WriteReg(0x3E, 0xFF);  //Maximum transmit power - write 0xFF to 0x3E (PATABLE)
  DEBUG_PRINTLN(" - Done");
}

void Ansulta::serverLoop()
{
  //Some demo loop
  if (!p_address_found) {
    /*** Read adress from another remote wireless ***/
    /*** Push the button on the original remote ***/
  
    ReadAddressBytes(); //Read Address Bytes From a remote by sniffing its packets wireless
    if (p_address_found) {
      Serial.print("50%");
      light_ON_50();
      delay(1000);
      Serial.print("100%");
      light_ON_100();
      delay(1000);
      Serial.print("50%");
      light_ON_50();
      delay(1000);
      Serial.print("OFF%");
      light_OFF();
    }
  }
  // backup case if it is not work
  if (p_count_repeats > 0) {
    p_count_repeats--;
    switch(p_led_state) {
      case ON_50:
        SendCommand(AddressByteA,AddressByteB, Light_ON_50);
        break;
      case ON_100:
        SendCommand(AddressByteA,AddressByteB, Light_ON_100);
        break;
      case OFF:
        SendCommand(AddressByteA,AddressByteB, Light_OFF);
        break;
    }
  }
  /*** Send the command to pair the transformer with this remote ***/
  // SendCommand(AddressByteA,AddressByteB, Light_PAIR);
  // delay(1000);
}

bool Ansulta::valid_address() {
  return p_address_found;
}

bool Ansulta::set_address(byte addr_a, byte addr_b)
{
  AddressByteA = addr_a;
  AddressByteB = addr_b;
  p_address_found = (addr_a != 0x00 && addr_b != 0x00);
  return p_address_found;
}

byte Ansulta::get_address_a()
{
  return AddressByteA;
}

byte Ansulta::get_address_b()
{
  return AddressByteB;
}

void Ansulta::light_ON_50(int count)
{
  /*** Send the command to turn the light on 50% ***/
  p_count_repeats = REPEATS;
  p_led_state = ON_50;
  SendCommand(AddressByteA, AddressByteB, Light_ON_50, count);
  // delay(1000);
}

void Ansulta::light_ON_100(int count)
{
  /*** Send the command to turn the light on 50% ***/
  p_count_repeats = REPEATS;
  p_led_state = ON_100;
  SendCommand(AddressByteA, AddressByteB, Light_ON_100, count);
  // delay(1000);
}

void Ansulta::light_OFF(int count)
{
  /*** Send the command to turn the light off ***/
  p_count_repeats = REPEATS;
  p_led_state = OFF;
  SendCommand(AddressByteA, AddressByteB, Light_OFF, count);
  // delay(1000);
}


int Ansulta::get_state()
{
  return p_led_state;
}

void Ansulta::ReadAddressBytes()
{     //Read Address Bytes From a remote by sniffing its packets wireless
   byte tries=0;

   if(!p_first_info) {
    DEBUG_PRINTLN("Ansulta: Listening for an Address ");
    p_first_info = true;
   }
   
   while((tries < MAX_LEARN_TRIES) && (!p_address_found)){ //Try to listen for the address 200 times
      p_count_c++;
      if (p_count_c > 80) {
        p_count_c = 0;
        DEBUG_PRINTLN("c");
      } else {
        DEBUG_PRINT("c");
      }
      SendStrobe(CC2500_SRX);
      WriteReg(REG_IOCFG1,0x01);   // Switch MISO to output if a packet has been received or not
      delay(10);
      byte PacketLength = ReadReg(CC2500_FIFO);
      if (PacketLength > 1) {      
//      if (digitalRead(MISO)) {      
        byte recvPacket[PacketLength];
        if (PacketLength <= 8) {                       //A packet from the remote cant be longer than 8 bytes
          DEBUG_PRINTLN();
          DEBUG_PRINT("Ansulta: Packet received: ");
          DEBUG_FPRINT(PacketLength, DEC);
          DEBUG_PRINTLN(" bytes");
          for (byte i = 0; i < PacketLength; i++){    //Read the received data from CC2500
            recvPacket[i] = ReadReg(CC2500_FIFO);
            if (recvPacket[i] < 0x10) { DEBUG_PRINT("0"); }
            DEBUG_FPRINT(recvPacket[i], HEX);
          }
        }
          
        byte start=0;
        while((recvPacket[start] != 0x55) && (start < PacketLength)){   //Search for the start of the sequence
          start++;
        }
        if (recvPacket[start+1] == 0x01 && recvPacket[start+5] == 0xAA){   //If the bytes match an Ikea remote sequence
          p_address_found = true;
          AddressByteA = recvPacket[start+2];                // Extract the addressbytes
          AddressByteB = recvPacket[start+3];
          DEBUG_PRINTLN();
          DEBUG_PRINT("Ansulta: Address Bytes found: ");
          if (AddressByteA < 0x10) { DEBUG_PRINT("0"); }
          DEBUG_FPRINT(AddressByteA, HEX);
          if (AddressByteB < 0x10) { DEBUG_PRINT("0"); }
          DEBUG_FPRINTLN(AddressByteB, HEX);
        } 
        SendStrobe(CC2500_SIDLE);      // Needed to flush RX FIFO
        SendStrobe(CC2500_SFRX);       // Flush RX FIFO
      } 
      tries++;  //Another try has passed
   }
   if (p_address_found) {
     DEBUG_PRINTLN();
     DEBUG_PRINTLN("Ansulta: detected");
   }
}

byte Ansulta::ReadReg(byte addr)
{
  addr = addr + 0x80;
  digitalWrite(SS,LOW);
  while (digitalRead(MISO) == HIGH) {
    };
  byte x = SPI.transfer(addr);
  delay(10);
  byte y = SPI.transfer(0);
  digitalWrite(SS,HIGH);
  return y;  
}

void Ansulta::SendStrobe(byte strobe, unsigned int delay_after)
{
  digitalWrite(SS, LOW);
  while (digitalRead(MISO) == HIGH) {
  };
  SPI.transfer(strobe);
  digitalWrite(SS, HIGH);
  delayMicroseconds(delay_after);
}

void Ansulta::SendCommand(byte AddressByteA, byte AddressByteB, byte Command, int count)
{
    DEBUG_PRINT("Ansulta: Send command ");
    DEBUG_FPRINT(Command, HEX);
    DEBUG_PRINT(" to ");
    if (AddressByteA < 0x10) { DEBUG_PRINT("0"); }  //Print leading zero
    DEBUG_FPRINT(AddressByteA, HEX);
    if (AddressByteB < 0x10) { DEBUG_PRINT("0"); }
    DEBUG_FPRINT(AddressByteB, HEX);

    for (byte i = 0; i < count; i++) {       //Send 50 times
      DEBUG_PRINT("~");
      SendStrobe(CC2500_SIDLE, delayB);   //0x36 SIDLE Exit RX / TX, turn off frequency synthesizer and exit Wake-On-Radio mode if applicable.
      SendStrobe(CC2500_SFTX, delayB);    //0x3B SFTX Flush the TX FIFO buffer. Only issue SFTX in IDLE or TXFIFO_UNDERFLOW states.
      digitalWrite(SS,LOW);
      while (digitalRead(MISO) == HIGH) { };  //Wait untill MISO high
      SPI.transfer(0x7F);
      delayMicroseconds(delayA);
      SPI.transfer(0x06);
      delayMicroseconds(delayA);
      SPI.transfer(0x55);
      delayMicroseconds(delayA);
      SPI.transfer(0x01);                 
      delayMicroseconds(delayA);
      SPI.transfer(AddressByteA);                 //Address Byte A
      delayMicroseconds(delayA);
      SPI.transfer(AddressByteB);                 //Address Byte B
      delayMicroseconds(delayA);
      SPI.transfer(Command);                      //Command 0x01=Light OFF 0x02=50% 0x03=100% 0xFF=Pairing
      delayMicroseconds(delayA);
      SPI.transfer(0xAA);
      delayMicroseconds(delayA);
      SPI.transfer(0xFF);
      digitalWrite(SS,HIGH);
      SendStrobe(CC2500_STX, delayB);                 //0x35 STX In IDLE state: Enable TX. Perform calibration first if MCSM0.FS_AUTOCAL=1. If in RX state and CCA is enabled: Only go to TX if channel is clear
      delayMicroseconds(delayC);      //Longer delay for transmitting
    }
    DEBUG_PRINTLN(" - Done");
}


void Ansulta::WriteReg(byte addr, byte value)
{
  digitalWrite(SS,LOW);
  while (digitalRead(MISO) == HIGH) {
    };
  SPI.transfer(addr);
  delayMicroseconds(delayE);
  SPI.transfer(value);
  digitalWrite(SS,HIGH);
  delayMicroseconds(delayD);
}


void Ansulta::init_CC2500()
{
  WriteReg(REG_IOCFG2,VAL_IOCFG2);
  WriteReg(REG_IOCFG0,VAL_IOCFG0);
  WriteReg(REG_PKTLEN,VAL_PKTLEN);
  WriteReg(REG_PKTCTRL1,VAL_PKTCTRL1);
  WriteReg(REG_PKTCTRL0,VAL_PKTCTRL0);
  WriteReg(REG_ADDR,VAL_ADDR);
  WriteReg(REG_CHANNR,VAL_CHANNR);
  WriteReg(REG_FSCTRL1,VAL_FSCTRL1);
  WriteReg(REG_FSCTRL0,VAL_FSCTRL0);
  WriteReg(REG_FREQ2,VAL_FREQ2);
  WriteReg(REG_FREQ1,VAL_FREQ1);
  WriteReg(REG_FREQ0,VAL_FREQ0);
  WriteReg(REG_MDMCFG4,VAL_MDMCFG4);
  WriteReg(REG_MDMCFG3,VAL_MDMCFG3);
  WriteReg(REG_MDMCFG2,VAL_MDMCFG2);
  WriteReg(REG_MDMCFG1,VAL_MDMCFG1);
  WriteReg(REG_MDMCFG0,VAL_MDMCFG0);
  WriteReg(REG_DEVIATN,VAL_DEVIATN);
  WriteReg(REG_MCSM2,VAL_MCSM2);
  WriteReg(REG_MCSM1,VAL_MCSM1);
  WriteReg(REG_MCSM0,VAL_MCSM0);
  WriteReg(REG_FOCCFG,VAL_FOCCFG);
  WriteReg(REG_BSCFG,VAL_BSCFG);
  WriteReg(REG_AGCCTRL2,VAL_AGCCTRL2);
  WriteReg(REG_AGCCTRL1,VAL_AGCCTRL1);
  WriteReg(REG_AGCCTRL0,VAL_AGCCTRL0);
  WriteReg(REG_WOREVT1,VAL_WOREVT1);
  WriteReg(REG_WOREVT0,VAL_WOREVT0);
  WriteReg(REG_WORCTRL,VAL_WORCTRL);
  WriteReg(REG_FREND1,VAL_FREND1);
  WriteReg(REG_FREND0,VAL_FREND0);
  WriteReg(REG_FSCAL3,VAL_FSCAL3);
  WriteReg(REG_FSCAL2,VAL_FSCAL2);
  WriteReg(REG_FSCAL1,VAL_FSCAL1);
  WriteReg(REG_FSCAL0,VAL_FSCAL0);
  WriteReg(REG_RCCTRL1,VAL_RCCTRL1);
  WriteReg(REG_RCCTRL0,VAL_RCCTRL0);
  WriteReg(REG_FSTEST,VAL_FSTEST);
  WriteReg(REG_TEST2,VAL_TEST2);
  WriteReg(REG_TEST1,VAL_TEST1);
  WriteReg(REG_TEST0,VAL_TEST0);
  WriteReg(REG_DAFUQ,VAL_DAFUQ);
  WriteReg(0x003E,0xAA);  // FULL POWER?
}
