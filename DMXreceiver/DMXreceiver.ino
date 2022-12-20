// Arduino application for receiving DMX data from DMXtransmitter
// and controlling lights more or less directry via GPIO

#include <FastLED.h>
#include <SPI.h>
#include "RF24.h"
#include "radio_constants.h"

// Each receiver needs its own ID here
// This is used as a part of address in the nRF24 radio
// Transmitter sends data to radios 1..wireless_receiver_count

uint8_t sine_wave_pow_2[256] = {
0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 15, 16, 18, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 42, 44, 46, 49, 51, 54, 56, 59, 62, 64, 67, 70, 73, 76, 79, 81, 84, 87, 90, 93, 96, 99, 103, 106, 109, 112, 115, 118, 121, 124, 127, 131, 134, 137, 140, 143, 146, 149, 152, 156, 159, 162, 165, 168, 171, 174, 176, 179, 182, 185, 188, 191, 193, 196, 199, 201, 204, 206, 209, 211, 213, 216, 218, 220, 222, 224, 226, 228, 230, 232, 234, 236, 237, 239, 240, 242, 243, 245, 246, 247, 248, 249, 250, 251, 252, 252, 253, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 254, 253, 252, 252, 251, 250, 249, 248, 247, 246, 245, 243, 242, 240, 239, 237, 236, 234, 232, 230, 228, 226, 224, 222, 220, 218, 216, 213, 211, 209, 206, 204, 201, 199, 196, 193, 191, 188, 185, 182, 179, 176, 174, 171, 168, 165, 162, 159, 156, 152, 149, 146, 143, 140, 137, 134, 131, 128, 124, 121, 118, 115, 112, 109, 106, 103, 99, 96, 93, 90, 87, 84, 81, 79, 76, 73, 70, 67, 64, 62, 59, 56, 54, 51, 49, 46, 44, 42, 39, 37, 35, 33, 31, 29, 27, 25, 23, 21, 19, 18, 16, 15, 13, 12, 10, 9, 8, 7, 6, 5, 4, 3, 3, 2, 1, 1, 0, 0, 0, 0, 0
};

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
#define NUM_LEDS 297
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

static const CRGB LED_OFF = CRGB(0, 0, 0);
static const CRGB LED_RED = CRGB(255, 0, 0);
static const CRGB LED_BLUE = CRGB(0, 0, 255);

void loop() {
 // timing stuff
 #define speed 4
  static uint8_t t = 0;
  static uint8_t u = 0;
  if (radio.available()) {
    uint8_t data[4];
    bool done = false;

    while (!done) {
      // transmitter sends its data as
      // (target, r, g, b), each value is uint8

      done = radio.read(&data, sizeof(data));

      uint8_t state = data[0];
      uint8_t r = data[1];
      uint8_t g = data[2];
      uint8_t b = data[3];

      //uint16_t lit_led_count = ((NUM_LEDS) * (uint32_t)(lit)) / 255;

      if (state == 0)
      {
        for (uint16_t i = 0; i < NUM_LEDS; i++)
        {
          leds[i] = LED_OFF;
        }
      }
      // This case handles both "stand by" with loading state 0 (state 1-10)
      // and the actual loading bars
      else if (state <= 210)
      {
        // Loading bar led count
        uint16_t lit_led_count = 0;
        if (state > 10)
        {
          // lit per side
          lit_led_count = state - 10;
        }

        for (uint16_t i = 0; i < NUM_LEDS/2; i++)
        {
          uint8_t intensity = sine_wave_pow_2[(u - 137*i)%256];
          //intensity = 10 + intensity >> 1;
          if (i < lit_led_count)
          {
            leds[NUM_LEDS - i - 1] = LED_RED;
            leds[i] = LED_RED;
          }
          else
          {
            uint8_t r2 = (r * intensity) >> 8;
            uint8_t g2 = (g * intensity) >> 8;
            uint8_t b2 = (b * intensity) >> 8;
            //leds[i] = CRGB(intensity >> 7, intensity >> 5, intensity >> 3);
            leds[NUM_LEDS - i - 1] = CRGB(r2, g2, b2);
            leds[i] = CRGB(r2, g2, b2);
            //leds[i] = CRGB(200, g2, b2);
          }
        }
      }
      else
      {
        for (uint16_t i = 0; i < NUM_LEDS; i++)
        {
          leds[i] = LED_BLUE;
        }
      }

      FastLED.show();
    }
  }
  else {
    delay(1);
  }

  if (++t > speed) {
    ++u;
    t = 0;
  };

}
