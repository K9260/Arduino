
#include <FastLED.h>

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    2
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS    135
#define MIC_PIN A0
#define LEDset 1



CRGB leds[NUM_LEDS];

#define BRIGHTNESS 255


void setup() {
  delay(500);
  pinMode(MIC_PIN, INPUT);
  Serial.begin(9600);
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  //Serial.println("Starting up");
}


int currentVal = 0; // value the mic is outputting
int previousVal = 340; // the mic gives this value by default so I it is good to initialize with this
//for other microphone you might want to to use other value
int highBeat = 60;
int mediumBeat = 40;
uint8_t gHue = 100; // rotating "base color" used by many of the patterns
uint8_t luma;
boolean reverseDir = false;
uint8_t index = 0;

void loop()
{

  currentVal = analogRead(MIC_PIN);
  EVERY_N_SECONDS(10){
  int ran = random(1, 6);
  if (ran == 2) 
    index = 1;
  else
  index = 0;
  }
  switch (index) {
    case 0: printLEDS();
      break;
    case 1: printLEDS_center();
      break;
  }

  previousVal = currentVal;

}

void addHue() {
  EVERY_N_MILLISECONDS(10) {

    if (gHue > 254) {
      reverseDir = true;
    }
    if (gHue < 100) {
      reverseDir = false;
    }
    if (reverseDir) {
      gHue--;
    }
    else {
      gHue++;
    }
  }
}

void printLEDS() {
  //Serial.println(val);
  //randomize if the leds should be driven on or off
  int r = random(1, 3);
  uint8_t b;
  if (currentVal > previousVal + funcParab(currentVal) && currentVal > 330) {
    b = 250;
    if (r == 1)
      addHue();
  }
  else if (currentVal > previousVal + funcParab(currentVal - 8) && currentVal > 330) {
    b = 80;
    if (r == 1)
      addHue();
  }
  else
    b = 0;
  uint8_t hue = random(-18, 18);
  hue += gHue;
  
  for (int i = 0; i < LEDset; i++) {
    leds[i] = CHSV(hue, 250, b);
  }
 
  //every led is pushed 1 step more further
  //example led[1] = led[0]
  //next loop >>
  //        led[2] = led[1]
  for (int i = NUM_LEDS - 1; i >= LEDset; i--) {
    leds[i] = leds[i - LEDset];
  }
  FastLED.show();
  delay(10);
}

void printLEDS_center() {

  //Serial.println(val);
  uint8_t b;
  if (currentVal > previousVal + funcParab(currentVal) && currentVal > 330) {
    b = 250;
    addHue();
  }
  else if (currentVal > previousVal + funcParab(currentVal - 8) && currentVal > 330) {
    b = 140;
    addHue();
  }
  else
    b = 0;
  uint8_t hue = random(-18, 18);
  hue += gHue;
  //LEDset amount of leds are given new colors starting from led[0] and going up
  for (int i = 0; i < LEDset; i++) {
    leds[NUM_LEDS / 2 + i] = CHSV(hue, 250, b);
    leds[(NUM_LEDS / 2 - i) - 1] = CHSV(hue, 250, b);
    fadeToBlackBy(leds, NUM_LEDS, 13);
  }

  //every led is pushed 1 step more further so it looks like the led is moving
  //example led[1] = led[0]

  for (int i = NUM_LEDS - 1; i >= (NUM_LEDS / 2); i--) {
    leds[i] = leds[i - LEDset];
  }
  for (int i = 0; i <= (NUM_LEDS / 2); i++) {
    leds[i] = leds[i + LEDset];
  }

  FastLED.show();
  delay(15);
}

//I recommend googling fscale function and replacing this with fscale if you use this in the future
float funcParab(float x) {
  //parabolic function that I use to calculate new value for volume peak detection
  float RTCVar;
  float a = 1.0 / 230.0;
  float c = 262.0;
  RTCVar = (a * (pow(x, 2))) - (2.2 * x) + c;
  //Serial.println(RTCVar);
  return RTCVar;
}
