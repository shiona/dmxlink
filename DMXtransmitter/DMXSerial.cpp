// - - - - -
// DMXSerial - A Arduino library for sending and receiving DMX using the builtin serial hardware port.
// DMXSerial.cpp: Library implementation file
//
// Copyright (c) 2011 by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
//
// Documentation and samples are available at http://www.mathertel.de/Arduino
// 25.07.2011 creation of the DMXSerial library.
// 10.09.2011 fully control the serial hardware register
//            without using the Arduino Serial (HardwareSerial) class to avoid ISR implementation conflicts.
// 01.12.2011 include file changed to work with the Arduino 1.0 environment
// 28.12.2011 unused variable DmxCount removed
// 10.05.2012 added method noDataSince to check how long no packet was received
// 04.06.2012: set UCSRnA = 0 to use normal speed operation
// 30.07.2012 corrected TX timings with UDRE and TX interrupts
//            fixed bug in 512-channel RX
// 26.03.2013 #defines for the interrupt vector names
//            auto-increase _dmxMaxChannel
// 15.05.2013 Arduino Leonard and Arduino MEGA compatibility
// 19.05.2013 ATmega8 compatibility (beta)
// 24.08.2013 Optimizations for speed and size.
//            Removed some "volatile" annotations. 
// - - - - -

#include "Arduino.h"

#include "DMXSerial.h"
#include <avr/interrupt.h>

// ----- Debugging -----

// to debug on an oscilloscope, enable this
#undef SCOPEDEBUG
#ifdef SCOPEDEBUG
#define DmxTriggerPin 4  // low spike at beginning of start byte
#define DmxISRPin 3 // low during interrupt service routines
#endif

// ----- Constants -----

// Define port & bit values for Hardware Serial Port.
// The library works unchanged with the Arduino 2009, UNO, MGEA 2560 and Leonardo boards.
// The Arduino MGEA 2560 boards use the serial port 0 on pins 0 an 1.
// The Arduino Leonardo will use serial port 1, also on pins 0 an 1. (on the 32u4 boards the first USART is USART1)
// This is consistent to the Layout of the Arduino DMX Shield http://www.mathertel.de/Arduino/DMXShield.aspx.

// For using the serial port 1 on a Arduino MEGA 2560 board, enable the following DMX_USE_PORT1 definition.
// #define DMX_USE_PORT1

#if !defined(DMX_USE_PORT1) && defined(USART_RXC_vect)
// These definitions are used on ATmega8 boards
#define UCSRnA UCSRA  // Control and Status Register A
#define TXCn   TXC    // Transmit buffer clear

#define UCSRnB UCSRB  // USART Control and Status Register B

#define RXCIEn RXCIE  // Enable Receive Complete Interrupt
#define TXCIEn TXCIE  // Enable Transmission Complete Interrupt
#define UDRIEn UDRIE  // Enable Data Register Empty Interrupt
#define RXENn  RXEN   // Enable Receiving
#define TXENn  TXEN   // Enable Sending

#define UCSRnC UCSRC  // Control and Status Register C
#define USBSn  USBS   // Stop bit select 0=1bit, 1=2bits
#define UCSZn0 UCSZ0  // Character size 00=5, 01=6, 10=7, 11=8 bits
#define UPMn0  UPM0   // Parity setting 00=N, 10=E, 11=O

#define UBRRnH UBRRH  // USART Baud Rate Register High
#define UBRRnL UBRRL  // USART Baud Rate Register Low

#define UDRn   UDR    // USART Data Register
#define UDREn  UDRE   // USART Data Ready
#define FEn    FE     // Frame Error

#define USARTn_RX_vect   USART_RXC_vect  // Interrupt Data received
#define USARTn_TX_vect   USART_TXC_vect  // Interrupt Data sent
#define USARTn_UDRE_vect USART_UDRE_vect // Interrupt Data Register empty


#elif !defined(DMX_USE_PORT1) && defined(USART_RX_vect)
// These definitions are used on ATmega168p and ATmega328p boards
// like the Arduino Diecimila, Duemilanove, 2009, Uno
#define UCSRnA UCSR0A
#define TXCn   TXC0
#define UCSRnB UCSR0B
#define RXCIEn RXCIE0
#define TXCIEn TXCIE0
#define UDRIEn UDRIE0
#define RXENn  RXEN0
#define TXENn  TXEN0
#define UCSRnC UCSR0C
#define USBSn  USBS0
#define UCSZn0 UCSZ00
#define UPMn0  UPM00
#define UBRRnH UBRR0H
#define UBRRnL UBRR0L
#define UDRn   UDR0
#define UDREn  UDRE0
#define FEn    FE0
#define USARTn_RX_vect   USART_RX_vect
#define USARTn_TX_vect   USART_TX_vect
#define USARTn_UDRE_vect USART_UDRE_vect

#elif !defined(DMX_USE_PORT1) && defined(USART0_RX_vect)
// These definitions are used on ATmega1280 and ATmega2560 boards
// like the Arduino MEGA boards
#define UCSRnA UCSR0A
#define TXCn   TXC0
#define UCSRnB UCSR0B
#define RXCIEn RXCIE0
#define TXCIEn TXCIE0
#define UDRIEn UDRIE0
#define RXENn  RXEN0
#define TXENn  TXEN0
#define UCSRnC UCSR0C
#define USBSn  USBS0
#define UCSZn0 UCSZ00
#define UPMn0  UPM00
#define UBRRnH UBRR0H
#define UBRRnL UBRR0L
#define UDRn   UDR0
#define UDREn  UDRE0
#define FEn    FE0
#define USARTn_RX_vect   USART0_RX_vect
#define USARTn_TX_vect   USART0_TX_vect
#define USARTn_UDRE_vect USART0_UDRE_vect

#elif defined(DMX_USE_PORT1) || defined(USART1_RX_vect)
// These definitions are used for using serial port 1
// on ATmega32U4 boards like Arduino Leonardo, Esplora
// You can use it on other boards with USART1 by enabling the DMX_USE_PORT1 port definition
#define UCSRnA UCSR1A
#define TXCn   TXC1
#define UCSRnB UCSR1B
#define RXCIEn RXCIE1
#define TXCIEn TXCIE1
#define UDRIEn UDRIE1
#define RXENn  RXEN1
#define TXENn  TXEN1
#define UCSRnC UCSR1C
#define USBSn  USBS1
#define UCSZn0 UCSZ10
#define UPMn0  UPM10
#define UBRRnH UBRR1H
#define UBRRnL UBRR1L
#define UDRn   UDR1
#define UDREn  UDRE1
#define FEn    FE1
#define USARTn_RX_vect   USART1_RX_vect
#define USARTn_TX_vect   USART1_TX_vect
#define USARTn_UDRE_vect USART1_UDRE_vect

#endif


// formats for serial transmission
#define SERIAL_8N1  ((0<<USBSn) | (0<<UPMn0) | (3<<UCSZn0))
#define SERIAL_8N2  ((1<<USBSn) | (0<<UPMn0) | (3<<UCSZn0))
#define SERIAL_8E1  ((0<<USBSn) | (2<<UPMn0) | (3<<UCSZn0))
#define SERIAL_8E2  ((1<<USBSn) | (2<<UPMn0) | (3<<UCSZn0))

// the break timing is 10 bits (start + 8 data + parity) of this speed
// the mark-after-break is 1 bit of this speed plus approx 6 usec
// 100000 bit/sec is good: gives 100 usec break and 16 usec MAB
// 1990 spec says transmitter must send >= 92 usec break and >= 12 usec MAB
// receiver must accept 88 us break and 8 us MAB
#define BREAKSPEED     100000
#define DMXSPEED       250000
#define BREAKFORMAT    SERIAL_8E1
#define DMXFORMAT      SERIAL_8N2

// ----- Enumerations -----

// State of receiving DMX Bytes
typedef enum {
  IDLE, BREAK, DATA
} DMXReceivingState;

// ----- Macros -----

// calculate prescaler from baud rate and cpu clock rate at compile time
// nb implements rounding of ((clock / 16) / baud) - 1 per atmega datasheet
#define Calcprescale(B)     ( ( (((F_CPU)/8)/(B)) - 1 ) / 2 )

// ----- DMXSerial Private variables -----
// These variables are not class members because they have to be reached by the interrupt implementations.
// don't use these variable from outside, use the appropriate methods.

DMXMode  _dmxMode; //  Mode of Operation

uint8_t _dmxRecvState;  // Current State of receiving DMX Bytes
int     _dmxChannel;  // the next channel byte to be sent.

volatile unsigned int  _dmxMaxChannel = 32; // the last channel used for sending (1..32).
volatile unsigned long _dmxLastPacket = 0; // the last time (using the millis function) a packet was received.

bool _dmxUpdated = true; // is set to true when new data arrived.
dmxUpdateFunction _dmxOnUpdateFunc = NULL;

// Array of DMX values (raw).
// Entry 0 will never be used for DMX data but will store the startbyte (0 for DMX mode).
uint8_t  _dmxData[DMXSERIAL_MAX+1];

// This pointer will point to the next byte in _dmxData;
uint8_t *_dmxDataPtr;

// This pointer will point to the last byte in _dmxData;
uint8_t *_dmxDataLastPtr;

// Create a single class instance. Multiple class instances (multiple simultaneous DMX ports) are not supported.
DMXSerialClass DMXSerial;


// ----- forwards -----

void _DMXSerialBaud(uint16_t baud_setting, uint8_t format);
void _DMXSerialWriteByte(uint8_t data);


// ----- Class implementation -----

// (Re)Initialize the specified mode.
// The mode parameter should be a value from enum DMXMode.
void DMXSerialClass::init (int mode)
{
#ifdef SCOPEDEBUG
  pinMode(DmxTriggerPin, OUTPUT);
  pinMode(DmxISRPin, OUTPUT);
#endif

  // initialize global variables
  _dmxMode = DMXNone;
  _dmxRecvState= IDLE; // initial state
  _dmxChannel = 0;
  _dmxDataPtr = _dmxData;
  _dmxLastPacket = millis(); // remember current (relative) time in msecs.

  // initialize the DMX buffer
//  memset(_dmxData, 0, sizeof(_dmxData));
  for (int n = 0; n < DMXSERIAL_MAX+1; n++)
    _dmxData[n] = 0;

  // now start
  _dmxMode = (DMXMode)mode;

  if (_dmxMode == DMXController) {
    // Setup external mode signal
    pinMode(DmxModePin, OUTPUT); // enables pin 2 for output to control data direction
    digitalWrite(DmxModePin, DmxModeOut); // data Out direction

    // Setup Hardware
    // Enable transmitter and interrupt
    UCSRnB = (1<<TXENn) | (1<<TXCIEn);

    // Start sending a BREAK and loop (forever) in UDRE ISR
    _DMXSerialBaud(Calcprescale(BREAKSPEED), BREAKFORMAT);
    _DMXSerialWriteByte((uint8_t)0);
    _dmxMaxChannel = 32; // The default in Controller mode is sending 32 channels.

  } else if (_dmxMode == DMXReceiver) {
    // Setup external mode signal
    pinMode(DmxModePin, OUTPUT); // enables pin 2 for output to control data direction
    digitalWrite(DmxModePin, DmxModeIn); // data in direction

    // Setup Hardware
    // Enable receiver and Receive interrupt
    UCSRnB = (1<<RXENn) | (1<<RXCIEn);
    _DMXSerialBaud(Calcprescale(DMXSPEED), DMXFORMAT); // Enable serial reception with a 250k rate

    _dmxMaxChannel = DMXSERIAL_MAX; // The default in Receiver mode is reading all possible 512 channels.
    _dmxDataLastPtr = _dmxData + _dmxMaxChannel;

  } else {
    // Enable receiver and transmitter and interrupts
    // UCSRnB = (1<<RXENn) | (1<<TXENn) | (1<<RXCIEn) | (1<<UDRIEn);

  } // if
} // init()


// Set the maximum used channel.
// This method can be called any time before or after the init() method.
void DMXSerialClass::maxChannel(int channel)
{
  if (channel < 1) channel = 1;
  if (channel > DMXSERIAL_MAX) channel = DMXSERIAL_MAX;
  _dmxMaxChannel = channel;
  _dmxDataLastPtr = _dmxData + channel;
} // maxChannel


// Read the current value of a channel.
uint8_t DMXSerialClass::read(int channel)
{
  // adjust parameter
  if (channel < 1) channel = 1;
  if (channel > DMXSERIAL_MAX) channel = DMXSERIAL_MAX;
  // read value from buffer
  return(_dmxData[channel]);
} // read()


// Write the value into the channel.
// The value is just stored in the sending buffer and will be picked up
// by the DMX sending interrupt routine.
void DMXSerialClass::write(int channel, uint8_t value)
{
  // adjust parameters
  if (channel < 1) channel = 1;
  if (channel > DMXSERIAL_MAX) channel = DMXSERIAL_MAX;
  if (value < 0)   value = 0;
  if (value > 255) value = 255;

  // store value for later sending
  _dmxData[channel] = value;

  // Make sure we transmit enough channels for the ones used
  if (channel > _dmxMaxChannel) {
    _dmxMaxChannel = channel;
    _dmxDataLastPtr = _dmxData + _dmxMaxChannel;
  } // if
} // write()


// Return the DMX buffer of unsave direct but faster access 
uint8_t *DMXSerialClass::getBuffer()
{
  return(_dmxData);
} // getBuffer()


// Calculate how long no data packet was received
unsigned long DMXSerialClass::noDataSince()
{
  unsigned long now = millis();
  return(now - _dmxLastPacket);
} // noDataSince()


// save function for the onUpdate callback
void DMXSerialClass::attachOnUpdate(dmxUpdateFunction newFunction)
{
  _dmxOnUpdateFunc = newFunction;
} // attachOnUpdate



bool DMXSerialClass::dataUpdated()
{
  return(_dmxUpdated);
}

void DMXSerialClass::resetUpdated()
{
  _dmxUpdated = false;
}


// Terminate operation
void DMXSerialClass::term(void)
{
  // Disable all USART Features, including Interrupts
  UCSRnB = 0;
} // term()


// ----- internal functions and interrupt implementations -----


// Initialize the Hardware serial port with the given baud rate
// using 8 data bits, no parity, 2 stop bits for data
// and 8 data bits, even parity, 1 stop bit for the break
void _DMXSerialBaud(uint16_t baud_setting, uint8_t format)
{
  // assign the baud_setting to the USART Baud Rate Register
  UCSRnA = 0;                 // 04.06.2012: use normal speed operation
  UBRRnH = baud_setting >> 8;
  UBRRnL = baud_setting;

  // 2 stop bits and 8 bit character size, no parity
  UCSRnC = format;
} // _DMXSerialBaud

// send the next byte after current byte was sent completely.
void _DMXSerialWriteByte(uint8_t data)
{
  // putting data into buffer sends the data
  UDRn = data;
} // _DMXSerialWrite


// This Interrupt Service Routine is called when a byte or frame error was received.
// In DMXController mode this interrupt is disabled and will not occur.
// In DMXReceiver mode when a byte was received it is stored to the dmxData buffer.
ISR(USARTn_RX_vect)
{
  uint8_t  USARTstate = UCSRnA;    // get state before data!
  uint8_t  DmxByte    = UDRn;    // get data
  uint8_t  DmxState   = _dmxRecvState;  //just load once from SRAM to increase speed

#ifdef SCOPEDEBUG
  digitalWrite(DmxISRPin, LOW);
#endif

  if (USARTstate & (1<<FEn)) {    //check for break
    _dmxRecvState = BREAK; // break condition detected.
    // _dmxChannel = 0;       // The next data byte is the start byte
    _dmxDataPtr = _dmxData;
      
  } else if (DmxState == BREAK) {
    if (DmxByte == 0) {
      // normal DMX start code (0) detected
#ifdef SCOPEDEBUG
      digitalWrite(DmxTriggerPin, LOW);
      digitalWrite(DmxTriggerPin, HIGH);
#endif
      _dmxRecvState = DATA;  
      _dmxLastPacket = millis(); // remember current (relative) time in msecs.
      // _dmxChannel++;       // start with channel # 1
      _dmxDataPtr++;

    } else {
      // This might be a RDM or customer DMX command -> not implemented so wait for next BREAK !
      _dmxRecvState = IDLE;
    } // if

  } else if (DmxState == DATA) {
    // check for new data
    if (*_dmxDataPtr != DmxByte) {
      _dmxUpdated = true;
      // store data
      *_dmxDataPtr = DmxByte;  //_dmxData[_dmxChannel] = DmxByte; store received data into dmx data buffer.
    } // if

    if (_dmxDataPtr == _dmxDataLastPtr) { // all channels received.
      _dmxRecvState = IDLE; // wait for next break

      if ((_dmxUpdated) && (_dmxOnUpdateFunc))
        _dmxOnUpdateFunc();
      _dmxUpdated = false;
    } // if
    // _dmxChannel++;
    _dmxDataPtr++;

  } // if

#ifdef SCOPEDEBUG
  digitalWrite(DmxISRPin, HIGH);
#endif
} // ISR(USARTn_RX_vect)


// Interrupt service routines that are called when the actual byte was sent.
// When changing speed (for sending break and sending start code) we use TX finished interrupt
// which occurs shortly after the last stop bit is sent
// When staying at the same speed (sending data bytes) we use data register empty interrupt
// which occurs shortly after the start bit of the *previous* byte
// When sending a DMX sequence it just takes the next channel byte and sends it out.
// In DMXController mode when the buffer was sent completely the DMX sequence will resent, starting with a BREAK pattern.
// In DMXReceiver mode this interrupt is disabled and will not occur.
ISR(USARTn_TX_vect)
{
#ifdef SCOPEDEBUG
  digitalWrite(DmxISRPin, LOW);
#endif
  if ((_dmxMode == DMXController) && (_dmxChannel == -1)) {
    // this interrupt occurs after the stop bits of the last data byte
    // start sending a BREAK and loop forever in ISR
    _DMXSerialBaud(Calcprescale(BREAKSPEED), BREAKFORMAT);
    _DMXSerialWriteByte((uint8_t)0);
    _dmxChannel = 0;

  } else if (_dmxChannel == 0) {
    // this interrupt occurs after the stop bits of the break byte
    // now back to DMX speed: 250000baud
    _DMXSerialBaud(Calcprescale(DMXSPEED), DMXFORMAT);
    // take next interrupt when data register empty (early)
    UCSRnB = (1<<TXENn) | (1<<UDRIEn);
    // write start code
    _DMXSerialWriteByte((uint8_t)0);
    _dmxChannel = 1;

#ifdef SCOPEDEBUG
    digitalWrite(DmxTriggerPin, LOW);
    digitalWrite(DmxTriggerPin, HIGH);
#endif
  } // if

#ifdef SCOPEDEBUG
  digitalWrite(DmxISRPin, HIGH);
#endif
} // ISR(USARTn_TX_vect)


// this interrupt occurs after the start bit of the previous data byte
ISR(USARTn_UDRE_vect)
{
#ifdef SCOPEDEBUG
  digitalWrite(DmxISRPin, LOW);
#endif
  _DMXSerialWriteByte(_dmxData[_dmxChannel++]);

  if (_dmxChannel > _dmxMaxChannel) {
    _dmxChannel   = -1; // this series is done. Next time: restart with break.
    // get interrupt after this byte is actually transmitted
    UCSRnB = (1<<TXENn) | (1<<TXCIEn);
  } // if
#ifdef SCOPEDEBUG
  digitalWrite(DmxISRPin, HIGH);
#endif
} // ISR(USARTn_UDRE_vect)

// The End
