#include "FastLED.h"
#define NUM_LEDS 60
// NUM_STRPS more than 7 won't work since we only have 7 frequency channels
#define NUM_STRPS 7
#define DATA_PIN 9
#define BRIGHT 45

CRGB leds[NUM_LEDS * NUM_STRPS];

// Declare Spectrum Shield pin connections
#define STROBE 4
#define RESET 6
#define DC_One A0
#define DC_Two A1

// Define spectrum variables
int freq_amp;
int Frequencies_One[7];
int Frequencies_Two[7];
int base;
int peak;
int p;
int i;
int divisor = 1024 / NUM_LEDS;
int litPix;

// Potentiometer Variables
int potPin = 2;
int potVal = 0;
// This is used to store and modify the color value
int adj_Val = 0;

void setup() {
  delay(2000);
  // Set Spectrum Shield pin configurations
  pinMode(STROBE, OUTPUT);
  pinMode(RESET, OUTPUT);
  pinMode(DC_One, INPUT);
  pinMode(DC_Two, INPUT);
  digitalWrite(STROBE, HIGH);
  digitalWrite(RESET, HIGH);

  // Initialize Spectrum Analyzers
  digitalWrite(STROBE, LOW);
  delay(1);
  digitalWrite(RESET, HIGH);
  delay(1);
  digitalWrite(STROBE, HIGH);
  delay(1);
  digitalWrite(STROBE, LOW);
  delay(1);
  digitalWrite(RESET, LOW);

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS * NUM_STRPS);

  Serial.begin(9600);
}

void loop() {
  Read_Pot();
  Read_Frequencies();
  // Graph_Frequencies();
  Graph_Frequencies_Adjusted();
  FastLED.show();
  delay(60);
}

void Read_Frequencies() {
  // Read frequencies for each band
  for (freq_amp = 0; freq_amp < 7; freq_amp++) {
    Frequencies_One[freq_amp] = analogRead(DC_One);
    Frequencies_Two[freq_amp] = analogRead(DC_Two);
    digitalWrite(STROBE, HIGH);
    digitalWrite(STROBE, LOW);
  }
}

void Graph_Frequencies() {
  for (i = 0; i < NUM_STRPS; i++) {
    // We set the base and peak of the string of leds we want to work with
    base = i * NUM_LEDS;
    peak = (i * NUM_LEDS) - 1 + NUM_LEDS;
    // We use the higher of the two stereo frequency values
    if (Frequencies_Two[i] > Frequencies_One[i]) {
      // The number of leds we want to be light up
      litPix = Frequencies_Two[i] / divisor;
      // We loop through and light up the leds for the strip we are on
      for (p = base; p <= base + litPix; p++) {
        leds[p] = CHSV(Frequencies_Two[i] / 4, 255, BRIGHT);
      }
      // We turn off the rest of the leds on that strip
      for (p = base + litPix + 1; p <= peak; p++) {
        leds[p] = CHSV(0, 0, 0);
      }
    } else {  // This should be the same as above, its used if frequecies_one is
              // the higher value
      litPix = Frequencies_One[i] / divisor;
      for (p = base; p <= base + litPix; p++) {
        leds[p] = CHSV(Frequencies_One[i] / 4, 255, BRIGHT);
      }
      for (p = base + litPix + 1; p <= peak; p++) {
        leds[p] = CHSV(0, 0, 0);
      }
    }
  }
}
void Graph_Frequencies_Adjusted() {
  // We divide the pot_val by 4 to get it in the range needed for setting the
  // color value.
  int pot_val = Read_Pot() / 4;
  for (i = 0; i < NUM_STRPS; i++) {
    // We take the average of the two channels.
    int freq_val = (Frequencies_Two[i] + Frequencies_One[i]) / 2;
    // We set the base and peak of the string of leds we want to work with
    base = i * NUM_LEDS;
    peak = (i * NUM_LEDS) - 1 + NUM_LEDS;
    // The number of leds we want to be light up
    litPix = freq_val / divisor;
    // adj_Val = Adjust_Color_Value(pot_val, Frequencies_Two[i]);
    // We loop through and light up the leds for the strip we are on
    // For the odd numbered strips we have to switch the order we light
    // pixels so they all start from the bottom.
    if (i % 2 != 0) {
      for (p = peak; p > peak - litPix; p--) {
        leds[p] = CHSV(pot_val, 255, BRIGHT);
      }
      // We turn off the rest of the leds on that strip
      for (p = base; p <= peak - litPix - 1; p++) {
        leds[p] = CHSV(0, 0, 0);
      }
    } else {
      for (p = base; p <= base + litPix; p++) {
        leds[p] = CHSV(pot_val, 255, BRIGHT);
      }
      // We turn off the rest of the leds on that strip
      for (p = base + litPix + 1; p <= peak; p++) {
        leds[p] = CHSV(0, 0, 0);
      }
    }
  }
}
void Graph_Frequencies_Middle() {
  int pot_val = 0;
  int freq_val = 0;
  int edge = 0;
  // We bring the value into a 256 bit range to pass to the fastLED library.
  pot_val = Read_Pot() / 4;

  for (i = 0; i < NUM_STRPS; i++) {
    // We average the frequencies to get our value.
    freq_val = (Frequencies_Two[i] + Frequencies_One[i]) / 2;
    // We translate the frequency into the number of LEDs we want to light up.
    litPix = freq_val / divisor;
    // We need an even number to display properly
    if (litPix % 2 != 0) {
      litPix += 1;
    }
    // We find the number of LEDs on the edges. We divide by two to make them
    // evenly spaced in the middle of the strip. (these will be turned off);
    edge = (NUM_LEDS - litPix) / 2;
    // We set the range for the LED strip we are working with.
    base = i * NUM_LEDS;
    peak = (i * NUM_LEDS) - 1 + NUM_LEDS;
    for (p = base; p <= base; p++) {
      // If p is in the edges turn the leds off, otherwise we turn them on using
      // the pot_val.
      if (p <= (base + edge) || p >= (peak - edge)) {
        leds[p] = CHSV(0, 0, 0);
      } else {
        leds[p] = CHSV(pot_val, 255, BRIGHT);
      }
    }
  }
}

int Read_Pot() { return analogRead(potPin); }

// This fuction expects both values to be a 1024 value
int Adjust_Color_Value(int val, int freq) {
  int store;
  store = (val / 4) + (freq / 4);
  if (store > 255) {
    store = store - 255;
  }
  return store;
}
