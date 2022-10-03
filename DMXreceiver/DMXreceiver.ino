// Softa ledien ajamiseen radiolla
// Koska helpompaa et vaan yks laite ajaa sitä DMX:ää

#include <SPI.h>
#include "RF24.h"
#include "radio_constants.h"

// HOX
// TÄÄ ON NYT SE VASTAANOTIN
// JOKAISELLE OMA NUMERO!!!!
const uint8_t RECEIVER_IDENTIFIER = 0x02;

////// Rauta //////
// Ledikrääsä
// Ledi väitti olevansa pinneissä R=3, G=5, B=6
// EI OLLUTKAA. YLLÄTYYYS
// Muuta vastaamaan ledistrippiä!

const uint8_t R_PIN[] = {6};
const uint8_t G_PIN[] = {5};
const uint8_t B_PIN[] = {3};
const uint8_t strip_count = 1;


void set_led(uint8_t led_pin, uint8_t brightness) {
  // jos meil on fetti nii se on näin päin
  char true_output = brightness;
  analogWrite(led_pin, true_output);
}

// Radio
// Pinneissä CE=9, CSN=10
RF24 radio(CHIP_ENABLE_PIN, CHIP_SELECT_PIN);

void setup() {
  // Ledit nolliin
  for(int subchannel = 1; subchannel <= strip_count; subchannel++) {
    pinMode(R_PIN[subchannel-1], OUTPUT);
    set_led(R_PIN[subchannel-1], 0);
  
    pinMode(G_PIN[subchannel-1], OUTPUT);
    set_led(G_PIN[subchannel-1], 0);
  
    pinMode(B_PIN[subchannel-1], OUTPUT);
    set_led(B_PIN[subchannel-1], 0);
  }

  // Radio päälle ja kuuntelemaan
  radio.begin();

  // Ei telemetriaa, pelkkä lukuputki
  // Maski | tunniste == radio-osoite
  uint64_t addr = RADIO_DATA_PIPE_MASK | RECEIVER_IDENTIFIER;
  radio.openReadingPipe(RADIO_DATA_PIPE_NUM, addr);
  radio.startListening();
}

void loop() {
  if (radio.available()) {
    uint8_t data[4];
    bool done = false;

    while (!done) {
      // Lue data
      // Formaatti on target, r, g, b
      // Jokainen on 8 bittiä unsigned
      done = radio.read(&data, sizeof(data));

      uint8_t subchannel = data[0];
      uint8_t r = data[1];
      uint8_t g = data[2];
      uint8_t b = data[3];
  
      if (subchannel <= strip_count) {
        // Kirjoita ledipinneihin
        set_led(R_PIN[subchannel-1], r);
        set_led(G_PIN[subchannel-1], g);
        set_led(B_PIN[subchannel-1], b);
      }
    }
  }
  else {
    // Nukutaan millisekunti
    delay(1);
  }
}
