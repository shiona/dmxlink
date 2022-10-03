// Softa DMX:n ajamiseen radioille
// Tai siis, DMX:n muuttaminen ledeille

#include <SPI.h>
#include "RF24.h"
#include "DMXSerial.h"

#include "radio_constants.h"

// list of numbers, each number signifying how many 4 DMX channel
// RGBI groups should be sent to each radio channel, in order.
// NOTE: each group need to be pre-mixed before sending.
const int wireless_receiver_count = 6;
const int wireless_channels[] = {1,1,1,1,1,1};

// Radio
// Pinneissä CE=9, CSN=10
RF24 radio(CHIP_ENABLE_PIN, CHIP_SELECT_PIN);
uint16_t dmx_offset;

bool send_data(uint8_t target, uint8_t subchannel,
               uint8_t r, uint8_t g, uint8_t b,
               uint8_t intensity) {
  bool result = true;
  
  // Lähetetääs data
  // Avataan eka oikee osoite
  uint64_t addr = RADIO_DATA_PIPE_MASK | target;

  uint8_t data[] = {subchannel,
                    (r * intensity) / 255,
                    (g * intensity) / 255,
                    (b * intensity) / 255};
  
  radio.stopListening();
  
  // Ilmeisesti tästä tulee paljon overheadia?
  // Toivottavasti ei tarvi muuttaa ihan hirveästi arvoja
  // samanaikaisesti
  radio.openWritingPipe(addr);
  if (!radio.write(data, sizeof(data))) {
    result = false;
  }
  
  radio.startListening();
  return result;
}

void setup() {
  // Radio päälle ja lähettämään
  radio.begin();
  radio.startListening();

  DMXSerial.init(DMXReceiver);
  
  // set some default values
  DMXSerial.write(1, 80);
  DMXSerial.write(2, 0);
  DMXSerial.write(3, 0);
  
  for (int i = 1; i <= 9; i++) {
    pinMode(i, INPUT);
    digitalWrite(i, HIGH);
  }

  delay(100);

  dmx_offset = 0;
  for (int i = 1; i <= 9; ++i) {
    dmx_offset |= (digitalRead(i) == LOW) << (i - 1);
  }
}

void loop() {
  int dmx_channel = dmx_offset;
  /*
  send_data(2, 2,
                DMXSerial.read(dmx_channel+1),
                DMXSerial.read(dmx_channel+2),
                DMXSerial.read(dmx_channel+3),
                DMXSerial.read(dmx_channel+4));
                */
  for(int wireless_channel = 1; 
      wireless_channel <= wireless_receiver_count; 
      wireless_channel++) {
    for(int subchannel = 1; 
        subchannel <= wireless_channels[wireless_channel-1];
        subchannel++) {          
      send_data(wireless_channel, subchannel,
                DMXSerial.read(dmx_channel+1),
                DMXSerial.read(dmx_channel+2),
                DMXSerial.read(dmx_channel+3),
                DMXSerial.read(dmx_channel+4));
      dmx_channel += 4;
    }
  }
}
