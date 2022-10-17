// Arduino application for receiving DMX data from DMXtransmitter
// and controlling lights more or less directry via GPIO

#include <FastLED.h>
#include <SPI.h>
#include "RF24.h"
#include "radio_constants.h"

// Each receiver needs its own ID here
// This is used as a part of address in the nRF24 radio
// Transmitter sends data to radios 1..wireless_receiver_count

const uint8_t RECEIVER_IDENTIFIER = 0x01;

// Current code supports any number of
// voltage controlled RGB strips.
// Hardware however supports only 3 channels of
// common anode (or cathode? need to check.)

const uint8_t R_PIN[] = {6};
const uint8_t G_PIN[] = {5};
const uint8_t B_PIN[] = {3};
const uint8_t strip_count = 1;

#define RGB_DATA_PIN 5
#define NUM_LEDS 300
CRGB leds[NUM_LEDS];

void set_led(uint8_t led_pin, uint8_t brightness) {
  // Depending on whether leds are common cathode or anode, the
  // pin should be driven higher or lower to make the channel brighter.
  char true_output = brightness;
  analogWrite(led_pin, true_output);
}

// Radio
// Pins CE=9, CSN=10 defined in radio_constants.h
RF24 radio(CHIP_ENABLE_PIN, CHIP_SELECT_PIN);

void setup() {
  // Initialize leds
  FastLED.addLeds<NEOPIXEL, RGB_DATA_PIN>(leds, NUM_LEDS);

  radio.begin();

  uint64_t addr = RADIO_DATA_PIPE_MASK | RECEIVER_IDENTIFIER;
  radio.openReadingPipe(RADIO_DATA_PIPE_NUM, addr);
  radio.startListening();
}

void loop() {
  if (radio.available()) {
    uint8_t data[4];
    bool done = false;

    while (!done) {
      // transmitter sends its data as
      // (target, r, g, b), each value is uint8

      done = radio.read(&data, sizeof(data));

      uint8_t intensity = data[0];
      uint8_t r = data[1];
      uint8_t g = data[2];
      uint8_t b = data[3];

      uint16_t lit_led_count = ((NUM_LEDS) * (uint32_t)(r)) / 255;

      for (uint16_t i = 0; i < NUM_LEDS; i++)
      {
        if (i < lit_led_count)
        {
          leds[i] = CRGB(2, 5, 20);
        }
        else
        {
          leds[i] = CRGB(0, 0, 0);
        }
      }

      FastLED.show();
    }
  }
  else {
    delay(1);
  }
}
