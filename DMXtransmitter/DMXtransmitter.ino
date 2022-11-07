// Arduino applicaion for transmitting data
// from DMX input to custom lights via nRF24 radio

#include <DMXSerial.h>
#include <SPI.h>
#include "RF24.h"

#include "radio_constants.h"

// Number of nRF24 radios receiving data
const int wireless_receiver_count = 2;

#define CHANNELS_PER_RECEIVER 4

// How many RGB tuples to send to each channel
const int wireless_channels[] = {1,1,1,1,1,1};

// Radio
// Pins CE=9, CSN=10 defined in radio_constants.h
RF24 radio(CHIP_ENABLE_PIN, CHIP_SELECT_PIN);

// How many dmx channels to skip before sending
// data to first radio. Allows wired lights to
// exist in the same DMX universe as this
// wireless bridge.
uint16_t dmx_offset;

uint8_t *dmx_buffer = 0;

bool send_data(uint8_t target,
               uint8_t r, uint8_t g, uint8_t b,
               uint8_t intensity) {
  bool result = true;

  // nRF24 radio address
  uint64_t addr = RADIO_DATA_PIPE_MASK | target;

  uint8_t data[] = {intensity,
  r,g,b};
                    //(r * intensity) / 255,
                    //(g * intensity) / 255,
                    //(b * intensity) / 255};

  //radio.stopListening();

  radio.openWritingPipe(addr);
  if (!radio.write(data, sizeof(data))) {
    result = false;
  }

  //radio.startListening();
  return result;
}

void setup() {
  radio.begin();
  // TODO: do we need to listen at all as transmitter?
  //radio.startListening();

  DMXSerial.init(DMXReceiver);
  //DMXSerial.maxChannel(wireless_receiver_count * CHANNELS_PER_RECEIVER);

  dmx_buffer = DMXSerial.getBuffer();

  // set some default values
  DMXSerial.write(1, 80);
  DMXSerial.write(2, 0);
  DMXSerial.write(3, 0);

  for (int i = 1; i <= 9; i++) {
    pinMode(i, INPUT);
    digitalWrite(i, HIGH);
  }

  delay(100);

  // Read the DMX address offset
  dmx_offset = 0;
  for (int i = 1; i <= 9; ++i) {
    dmx_offset |= (digitalRead(i) == LOW) << (i - 1);
  }

  DMXSerial.maxChannel(dmx_offset + wireless_receiver_count * CHANNELS_PER_RECEIVER);
}

void loop() {
  int dmx_channel = dmx_offset;

  for(int wireless_channel = 1;
      wireless_channel <= wireless_receiver_count;
      wireless_channel++) {
      send_data(wireless_channel,
                dmx_buffer[dmx_channel+1],
                dmx_buffer[dmx_channel+2],
                dmx_buffer[dmx_channel+3],
                dmx_buffer[dmx_channel+4]
                );
      /*
      send_data(wireless_channel,
                DMXSerial.read(dmx_channel+1),
                DMXSerial.read(dmx_channel+2),
                DMXSerial.read(dmx_channel+3),
                DMXSerial.read(dmx_channel+4));
                */
      dmx_channel += 4;
  }
}
