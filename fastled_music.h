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

CRGB leds[NUM_LEDS];

#define BRIGHTNESS 255


void setup() {
  delay(500);
  pinMode(MIC_PIN, INPUT);
 // Serial.begin(9600);
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  //Serial.println("Starting up");
}
int currentVal = 0; // value the mic is outputting
int previousVal = 340; // the mic gives this value by default so I it is good to initialize with this
                       //for other microphone you might want to to use other value
int timer = 0;  //"timer" is used so that after certain time the strip will go dark
              //counter is maybe more accurate term but 
int highBeat = 50;
int mediumBeat = 40;
int samples[8];
int i = 0;
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
uint8_t luma;





void loop()
{

  currentVal = analogRead(MIC_PIN);

  /*
    Serial.print(currentVal);
    Serial.print(" - ");
    Serial.print(previousVal);
    Serial.print(" = ");
    Serial.println(currentVal - previousVal);
  */


  // calculate average volume and from that
  // find good values for high and mediumbeat
  // these are most likely needed to be adjusted depending on your mic setting
  if ((currentVal - previousVal) > 0) {
    samples[i] = currentVal;
    i++;
  }
  if (i >= 7) {
    // Serial.println( average(samples, i));
    int avrg = average(samples, i);
    if (avrg < 345) {
      highBeat = 28;
      mediumBeat = 20;
    }
    else if (avrg < 350) {
      highBeat = 45;
      mediumBeat = 38;
    }
    else if (avrg < 355) {
      highBeat = 50;
      mediumBeat = 42;
    }
    else {
      highBeat = 60;
      mediumBeat = 50;
    }

   // Serial.println(highBeat);
  //  Serial.println(mediumBeat);
  
    i = 0;
  }
  delay(5);

  if (currentVal > previousVal + highBeat) {
    pickFastEffect();
    timer = 0;
  }
  else if (currentVal > previousVal + mediumBeat) {
    pickMediumEffect();
    timer = 0;
  } //if the so called "timer" is over 400, strip will go dark
  //timer gets +1 always when sprinkle() is called
  else if (timer < 400) {
    sprinkle();
  }
  else {
    FastLED.clear();
    FastLED.show();
  }
  previousVal = currentVal;

}
float average (int * array, int len)  // assuming array is int.
{
  long sum = 0L ;  // sum will be larger than an item, long for safety.
  for (int i = 0 ; i < len ; i++)
    sum += array [i] ;
  return  ((float) sum) / len ;  // average will be fractional, so float may be appropriate.
}
void addHue() {
  EVERY_N_MILLISECONDS(10) {
    gHue++;
  }
  if (gHue > 255) {
    gHue = 0;
  }
}
void pickFastEffect() {

  int temp = random(4);
  switch (temp) {
    case 1: oneColorFlash();
      break;
    case 2: rainbow();
      break;
    default: sprinkle();
  }
}
void pickMediumEffect() {
  int temp = random(6);
  switch (temp) {
    case 1: trailLeft();
      break;
    case 2: oneSideFlash();
      break;
    case 3: flash();
      break;
    case 4: sinelon();
      break;
    case 5: bpm(200);
      break;
    default: sprinkle();
  }
}

void oneSideFlash() {


  int startingPoint = 0;
  int lastLED = 0;

    int color = 0;
    //pick a random color


    for (int i = startingPoint; i < NUM_LEDS - 5 ; i += 10) {

      for (int a = i; a < i + 5; a++) {
        color += 3;
        if (color > 255)
          color = 0;
        fadeToBlackBy(leds, NUM_LEDS, 20);
        leds[a] = CHSV( color, 255, 250);
        lastLED = a; // this is only for fading the lights out
      }
      FastLED.show();
      delay(60);

     //this value needs to be changed for longer or shorter led strip
     //but it might crash the program if it is too big
      if (lastLED > 120) {
        do {
          //get light of leds
          luma = leds[lastLED].getLuma();
          fadeToBlackBy(leds, NUM_LEDS, 20);
          FastLED.show();
        } while (luma > 0); // fade until they are completely turned off
      }
    }
    FastLED.clear();
    if (startingPoint < 5)
      startingPoint = 5;
    else
      startingPoint = 0;


  
}
void oneColorFlash() {
  int color;
  //pick a random color
  color = random(255);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV( color, 255, 250);
  }
  FastLED.show();
  delay(150);

  do {
    //get light level of leds
    luma = leds[0].getLuma();
    fadeToBlackBy(leds, NUM_LEDS, 25);
    FastLED.show();
  } while (luma > 0);

}
void trailLeft() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV( gHue, 255, 192);
    fadeToBlackBy(leds, NUM_LEDS, 25);
    FastLED.show();
    delay(5);
    addHue();

  }
}


void flash() {
  int color = random(255);
  for (int b = NUM_LEDS / 2; b > 0 / 2; b--) {
    fadeToBlackBy(leds, NUM_LEDS, 10);

    for (int i = b; i < b + 1; i++) {
      leds[i] = CHSV(color, 255, 255);
    }

    for (int i = NUM_LEDS - b; i > (NUM_LEDS - b) - 1 ; i--) {
      leds[i] = CHSV( color, 255, 255);
    }
    FastLED.show();
    delay(5);
  }
}

void sprinkle() {

  fadeToBlackBy( leds, NUM_LEDS, 15);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(50), 250, 255);

  FastLED.show();
  // insert a delay to keep the framerate modest
  FastLED.delay(25);
  addHue();
  timer++;

}
void bpm(int limit)
{
  for (int i = 0;  i < limit; i++) {
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for ( int i = 0; i < NUM_LEDS; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
    FastLED.show();
  }
}
void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
  FastLED.show();
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  for (int i = 0; i < 200; i++) {
    fadeToBlackBy( leds, NUM_LEDS, 20);
    int pos = beatsin16( 13, 0, NUM_LEDS - 1 );
    leds[pos] += CHSV( gHue, 255, 192);
    delay(5);
    FastLED.show();
  }
}
