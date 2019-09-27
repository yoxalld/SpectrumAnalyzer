#include "FastLED.h"
#define NUM_LEDS 60
// NUM_STRPS more than 7 won't work since we only have 7 frequency channels
#define NUM_STRPS 7
#define DATA_PIN 9
#define BRIGHT 65

CRGB leds[NUM_LEDS * NUM_STRPS];

// Declare Spectrum Shield pin connections
#define STROBE 4
#define RESET 6
#define DC_One A0
#define DC_Two A1
// Rotary Encoder Pins
#define ROTARY_A 13
#define ROTARY_B 12
#define ROATRY_BUTTON 11

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

int Rot_Count = 0;
int Rot_State;
int Rot_Last_State;

// Potentiometer Variables
int potPin = 2;
int potVal = 0;
// This is used to store and modify the color value
int adj_Val = 0;

typedef void (*SimplePatternList[])();
SimplePatternList patterns = {Graph_Frequencies_Middle,
                              Graph_Frequencies_Adjusted, Graph_Frequencies};
int total_paterns = (sizeof(patterns) / sizeof((patterns)[0]));
uint8_t pattern_count = 0;

void setup() {
  delay(2000);
  // Set Spectrum Shield pin configurations
  pinMode(STROBE, OUTPUT);
  pinMode(RESET, OUTPUT);
  pinMode(DC_One, INPUT);
  pinMode(DC_Two, INPUT);
  // Rotary Encoder
  pinMode(ROTARY_A, INPUT);
  pinMode(ROTARY_B, INPUT);

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

  // Read initial state of Rotary Encoder
  Rot_Last_State = digitalRead(ROTARY_A);
}

void loop() {
  Read_Pot();
  Read_Frequencies();
  Read_Rotary();
  // Graph_Frequencies();
  // Graph_Frequencies_Adjusted();

  EVERY_N_MILLISECONDS(
      60) {  // FastLED based non-blocking delay to update/display the sequence.
    patterns[pattern_count]();
    // Graph_Frequencies_Middle();
    FastLED.show();
  }
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

void Read_Rotary() {
  Rot_State = digitalRead(ROTARY_A);
  if (Rot_State != Rot_Last_State) {
    if (digitalRead(ROTARY_B) != Rot_State) {
      nextPattern();
    } else {
      prevPattern();
    }
    Serial.print("Position: ");
    Serial.println(Rot_Count);
  }
  Rot_Last_State = Rot_State;
}

void Graph_Frequencies() {
  for (i = 0; i < NUM_STRPS; i++) {
    // We set the base and peak of the string of leds we want to work with
    base = i * NUM_LEDS;
    peak = (i * NUM_LEDS) - 1 + NUM_LEDS;
    int freq_val = (Frequencies_Two[i] + Frequencies_One[i]) / 2;
    litPix = freq_val / divisor;
    // We loop through and light up the leds for the strip we are on
    for (p = base; p <= base + litPix; p++) {
      leds[p] = CHSV(freq_val / 4, 255, BRIGHT);
    }
    // We turn off the rest of the leds on that strip
    for (p = base + litPix + 1; p <= peak; p++) {
      leds[p] = CHSV(0, 0, 0);
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
  // We bring the value into a 256 bit range to pass to the fastLED library.
  int pot_val = Read_Pot() / 4;
  for (i = 0; i < NUM_STRPS; i++) {
    // We average the frequencies to get our value.
    int freq_val = (Frequencies_Two[i] + Frequencies_One[i]) / 2;
    // We translate the frequency into the number of LEDs we want to light up.
    litPix = freq_val / divisor;
    // We need an even number to display properly
    if (litPix % 2 != 0) {
      litPix += 1;
    }
    // We find the number of LEDs on the edges. We divide by two to make them
    // evenly spaced in the middle of the strip. (these will be turned off);
    int edge = (NUM_LEDS - litPix) / 2;
    // We set the range for the LED strip we are working with.
    base = i * NUM_LEDS;
    peak = (i * NUM_LEDS) - 1 + NUM_LEDS;
    for (p = base; p <= peak; p++) {
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

void nextPattern() {
  // add one to the current pattern number, and wrap around at the end
  pattern_count = (pattern_count + 1) % total_paterns;
}
void prevPattern() {
  pattern_count = (pattern_count - 1);
  if (pattern_count > total_paterns) {
    pattern_count = total_paterns - 1;
  }
}
