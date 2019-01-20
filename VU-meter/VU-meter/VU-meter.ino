#include <FastLED.h>
#include <math.h>
FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    2
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS    36
#define MIC_MIN     0
#define MIC_MAX     50
#define NOISE       339
#define SAMPLES     50
#define MIN_VOL     15
#define MIC_PIN     A0
#define MIN_LEDS    6

CRGB leds[NUM_LEDS];

uint8_t hue = 0;

const int maxBeats = 10; //Min is 2 and value has to be divisible by two
const int maxBubbles = NUM_LEDS / 3; //Decrease if there is too much action going on
const int maxRipples = 6; //Min is 2 and value has to be divisible by two
const int maxTrails = 3;

struct bubble
{
  int brightness;
  int color;
  int pos;
  int velocity;
  int life;
  int maxLife;
  int maxVelocity;
  bool exist;

  bubble() {
    Init();
  }
  Move() {
    if (velocity > maxVelocity)
      velocity = maxVelocity;
    pos += velocity;
    life++;
    brightness -= 255 / maxLife;
    if (pos > NUM_LEDS - 1) {
      velocity *= -1;
      pos = NUM_LEDS - 1;
    }
    if (pos < 0) {
      velocity *= -1;
      pos = 0;
    }
    if (life > maxLife || brightness < 20)
      exist = false;
    if (!exist)
      Init();
  }
  NewColor() {
    color = hue + random(20);
    brightness = 255;
  }
  Init() {
    pos = random(0, NUM_LEDS);
    velocity = 1; // Increase or decrease if needed
    life = 0;
    maxLife = 80; //How many moves bubble can do
    exist = false;
    brightness = 255;
    color = hue + random(30);
    maxVelocity = 2;
  }
};
typedef struct bubble Bubble;

struct ripple
{
  int brightness;
  int color;
  int pos;
  int velocity;
  int life;
  int maxLife;
  float fade;
  bool exist;

  ripple() {
    Init(0.85, 20);
  }
  Move() {
    pos += velocity;
    life++;
    if (pos > NUM_LEDS - 1) {
      velocity *= -1;
      pos = NUM_LEDS - 1;
    }
    if (pos < 0) {
      velocity *= -1;
      pos = 0;
    }
    brightness *= fade; //Adjust accordinly to strip length
    if (life > maxLife || brightness < 50) exist = false;
    if (!exist) Init(0.85, 20);
  }
  Init(float Fade, int MaxLife) {
    pos = random(NUM_LEDS / 8, NUM_LEDS - NUM_LEDS / 8); //Avoid spawning too close to edge
    velocity = 1;
    life = 0;
    maxLife = MaxLife;
    exist = false;
    brightness = 255;
    color = hue;
    fade = Fade;
  }
};
typedef struct ripple Ripple;

int starIndex = 0;

struct star
{
  int brightness;
  int color;
  int pos;
  int OriginalPos;
  int life;
  int maxLife;
  bool exist;

  star() {
    Init();
    OriginalPos = starIndex;
    pos = OriginalPos;
    starIndex++;
  }
  Shine() {
    life++;
    brightness -= 255 / maxLife;
    if (life > maxLife || brightness <= 0) exist = false;
    if (brightness < 0) brightness = 0;
    if (pos < 0) pos = 0;
    if (pos > NUM_LEDS - 1) pos = NUM_LEDS - 1;
  }
  Init() {
    life = 0;
    maxLife = random(40, 60);
    exist = false;
    brightness = 255;
    color = 160;
    pos = OriginalPos;
  }
};
typedef struct star Star;

Ripple beat[maxBeats];
Ripple ripple[maxRipples];
Bubble bubble[maxBubbles];
Bubble trail[maxTrails];
Star star[NUM_LEDS];

int index = 0;
int gravityCounter = 0;
int sampleCount = 0;
int samples[SAMPLES];

uint8_t cBrightness;
uint8_t pBrightness = 0;

float MAX_VOL = MIC_MAX;
float sensitivity = 0.50; //Higher value = smaller sensitivity
float gravity = 1; //Higher value = leds fall back down faster
float topLED = 0;

bool reversed = false;
bool changeColor = false; //Does the function randomly change color every n seconds

void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  Serial.begin(115200);
  pinMode(MIC_PIN, INPUT);
  //Initialize samples
  for (int i = 0; i < SAMPLES; i++)
    samples[i] = MIC_MAX;
}
void loop() {

   vu();
  // dots();
  // split();
  // brush();
  // beats();
  // bubbles();
  // ripples();
  // trails();
  // flash();
  // stars();
  /*
    switch (index) {
      case 0: vu();
        break;
      case 1: dots();
        break;
      case 2: split();
        break;
      case 3: brush();
        break;
      case 4: beats();
        break;
      case 5: bubbles();
        break;
      case 6: ripples();
      break;
      case 7: trails();
      break;
      case 8: flash();
      break;
      case 9: stars();
    }
    EVERY_N_SECONDS(20) {
      index++;
      if (index > 9)
        index = 0;
    }*/
  EVERY_N_SECONDS(10) {
    if (changeColor)
      hue = random(256); //Every 10 seconds pick random color
  }
}
void vu() {
  int audio = readInput();
  int max_threshold = NUM_LEDS - 1;
  int min_threshold = 0;
  changeColor = true;
  MAX_VOL = audioMax(audio, 5);
  audio = fscale(MIC_MIN, MAX_VOL, min_threshold, max_threshold, audio, 0.5);
  if (audio < MIN_LEDS) audio = 0; //Avoid flickering caused by small interference
  for (int i = 0; i < audio; i++) {
    leds[i] = CHSV(hue + i * 2, 255, 255 - i);
  }
  if (audio >= topLED)
    topLED = audio;
  else
  topLED -= gravity;
  if(topLED < min_threshold) 
  topLED = min_threshold;
 
  leds[(int)topLED] = CHSV(220, 255, 255); //Pink led as top led with gravity
  FastLED.show();
  fadeToBlackBy(leds, NUM_LEDS, 160); //Looks smoother than using FastLED.clear()
  delay(30);
}

void dots() {
  int audio = readInput();
  int max_threshold = NUM_LEDS - 1;
  int min_threshold = 1;
  changeColor = true;
  MAX_VOL = audioMax(audio, 5);
  audio = fscale(MIC_MIN, MAX_VOL, min_threshold, max_threshold, audio, 0.5);

  if (audio >= topLED)
    topLED = audio;
  else
  topLED -= gravity;
  if(topLED < min_threshold) 
  topLED = min_threshold;
 
  leds[(int)topLED] = CHSV(hue, 255, 255); //Top led with gravity
  leds[(int)topLED - 1] = leds[(int)topLED];    //His friend :D

  gravityCounter++;
  if (gravityCounter > gravity)
    gravityCounter = 0;
  FastLED.show();
  FastLED.clear();
  delay(30);
}

void split() { //Vu meter starting from middle of strip and doing same effect to both directions
  int audio = readInput();
  int max_threshold = NUM_LEDS - 1;
  int min_threshold = NUM_LEDS / 2;
  changeColor = true;
  MAX_VOL = audioMax(audio, 5);
  audio = fscale(MIC_MIN, MAX_VOL, min_threshold, max_threshold, audio, 1.0);
  if (audio < MIN_LEDS / 2 + (NUM_LEDS / 2)) audio = min_threshold; //Avoid flickering caused by small interference
  for (int i = min_threshold; i < audio; i++) {
    leds[i] = CHSV(hue + i * 8, 250, 255 + (NUM_LEDS / 2) - i); //LED's drawn up, brightness gets lower as leds go higher
    leds[NUM_LEDS - i - 1] = leds[i]; //LEDS DRAWN DOWN
  }
  if (topLED <= audio)
    topLED = audio;
  else
  topLED -= (gravity / 2); //Divided by two because strip is split into half
  if(topLED < min_threshold) 
  topLED = min_threshold;

  leds[(int)topLED] = CHSV(220, 255, 255);         //Top led on top of strip (PINK)
  leds[NUM_LEDS - (int)topLED - 1] = leds[(int)topLED]; //On top of bottom half of strip

  FastLED.show();
  fadeToBlackBy(leds, NUM_LEDS, 160); //Looks smoother than using FastLED.clear()
  delay(30);
}

void brush() { // Swipes/brushes new color over the old one
  int audio = readInput();
  changeColor = false;
  MAX_VOL = audioMax(audio, 4);
  fill_solid(leds, NUM_LEDS, CHSV(hue, 255, 255));
  if (audio > MAX_VOL) {
    hue = random(255);
    if (hue < 127)
      reversed = true;
    else
      reversed = false;

    if (!reversed) {
      for (int i = 0; i < NUM_LEDS; i++) { //Draw one led at a time (UP)
        leds[i] = CHSV(hue, 255, 255);
        FastLED.show();
        delay(10);
      }
    }
    else {
      for (int i = NUM_LEDS - 1; i >= 0; i--) { //Draw one led at a time (DOWN)
        leds[i] = CHSV(hue, 255, 255);
        FastLED.show();
        delay(10);
      }
    }
  }
  FastLED.show();
  FastLED.clear();
}

void beats() {
  int audio = readInput();
  changeColor = true;
  MAX_VOL = audioMax(audio, 5);
  int newBeats = fscale(MIC_MIN, MAX_VOL * 0.8, 2, maxBeats, audio, 0.5); //Max amount of beats to be spawned this loop
  if (newBeats % 2 != 0) //Must be divisible by two
    newBeats--;

  for (int i = 0; i < newBeats; i += 2) {
    if (audio > MAX_VOL * sensitivity * 0.55 && !beat[i].exist) {
      int beatlife = random(3, 6); //Longer life = longer length
      beat[i].Init(0.75, beatlife); //Initialize fade and life
      beat[i].exist = true;
      beat[i].color += random(30);
      beat[i + 1] = beat[i]; //Everything except velocity is the same for other beats twin
      beat[i + 1].velocity *= -1; //because we want the other to go opposite direction
    }
  }
  for (int i = 0; i < maxBeats; i++) {
    if (beat[i].exist) {
      leds[beat[i].pos] = CHSV(beat[i].color, 255, beat[i].brightness);
      beat[i].Move();
    }
  }
  FastLED.show();
  fadeToBlackBy(leds, NUM_LEDS, 60);
  delay(30);
}
void bubbles() { //Spawns bubbles that move when audio peaks enough
  int audio = readInput();
  changeColor = true;
  MAX_VOL = audioMax(audio, 4);
  if (audio > MAX_VOL * sensitivity)
  {
    int randomBubble = random(maxBubbles);
    if (bubble[randomBubble].exist) {
      bubble[randomBubble].life /= 2; //Give that bubble longer life
      bubble[randomBubble].NewColor(); // And new color
    }
    else {
      bubble[randomBubble].exist = true;
      bubble[randomBubble].NewColor();
    }
  }
  for (int i = 0; i < maxBubbles; i++) {
    if (bubble[i].exist) {
      leds[bubble[i].pos] = CHSV(bubble[i].color, 255, bubble[i].brightness);
      bubble[i].Move();
    }
  }
  FastLED.show();
  FastLED.clear();
  delay(20);
}
void ripples() {
  int audio = readInput();
  changeColor = false;
  MAX_VOL = audioMax(audio, 5);

  int newRipples = fscale(MIC_MIN, MAX_VOL, 2, maxRipples, audio, 0.5); //Max amount of ripples to be spawned this loop
  if (newRipples % 2 != 0) //Must be divisible by two
    newRipples--;

  EVERY_N_SECONDS(1) {
    hue += 5; //Slight color rotation
  }
  int b = beatsin8(30, 70, 90); //Pulsing brightness for background (pulses,min,max)
  fill_solid(leds, NUM_LEDS, CHSV(hue, 255, b)); //Delete this line if you dont like background color
  for (int i = 0; i < newRipples; i += 2) {
    if (audio > MAX_VOL * sensitivity * 0.65 && !ripple[i].exist) {
      ripple[i].Init(0.85, 20); //Initiliaze just in case color has changed after last init
      ripple[i].exist = true;
      ripple[i + 1] = ripple[i]; //Everything except velocity is the same for other ripples twin
      ripple[i + 1].velocity *= -1; //because we want the other to go opposite direction
    }
  }
  for (int i = 0; i < maxRipples; i++) {
    if (ripple[i].exist) {
      leds[ripple[i].pos] = CHSV(ripple[i].color + 30, 180, ripple[i].brightness);
      ripple[i].Move();
    }
  }
  FastLED.show();
  FastLED.clear();
  delay(20);
}
void trails() { //Spawns trails that move
  int audio = readInput();
  changeColor = true;
  MAX_VOL = audioMax(audio, 4);
  if (audio > MAX_VOL * sensitivity)
  {
    int randomTrail = random(maxTrails);
    if (trail[randomTrail].exist) {
      trail[randomTrail].life /= 2; //Extend life of trail
      trail[randomTrail].NewColor(); // And give new color
    }
    else {
      trail[randomTrail].exist = true;
      trail[randomTrail].maxLife = 40;
    }
  }
  for (int i = 0; i < maxTrails; i++) {
    if (trail[i].exist) {
      leds[trail[i].pos] = CHSV(trail[i].color, 255, trail[i].brightness);
      trail[i].Move();
    }
  }
  FastLED.show();
  fadeToBlackBy(leds, NUM_LEDS, 100);
  delay(20);
}
void flash() { //Flashing strip
  int audio = readInput();
  changeColor = false;
  MAX_VOL = audioMax(audio, 4);
  cBrightness = fscale(MIC_MIN, MAX_VOL, 90, 255, audio, 0.5); //New brightness

  if (pBrightness * 2 < cBrightness && cBrightness > 120) { //Previous compared to current with some extra threshold to avoid flickering
    fill_solid(leds, NUM_LEDS, CHSV(hue, 255, cBrightness));
    hue += 3;
    pBrightness = cBrightness;
  }
  if (pBrightness < 20)
    pBrightness = 0;
  else
    pBrightness -= 10;

  FastLED.show();
  fill_solid(leds, NUM_LEDS, CHSV(hue, 255, pBrightness));
  delay(20);
}

void stars() {
  int audio = readInput();
  bool starsYoung = false;
  changeColor = false;
  MAX_VOL = audioMax(audio, 5);
  if (audio > MAX_VOL) {
    int color = random(140, 180);
    //Go through collection of stars and check if they have only existed less than half of their life
    for (int i = 0; i < NUM_LEDS; i++) {
      if (star[i].life < (star[i].maxLife / 2) && star[i].life > 0)
        starsYoung = true;
    }
    //Fill strip with stars
    for (int i = 0; i < NUM_LEDS; i++) {
      if (!starsYoung) {
        star[i].Init();
        star[i].color = color;
        leds[i] = CHSV(star[i].color, 255, star[i].brightness);
        star[i].exist = true;
      }
    }
  }
  for (int i = 0; i < NUM_LEDS; i += 2) {
    int randomStar = random(NUM_LEDS);
    if (star[i].exist || star[randomStar].exist) {
      leds[star[i].pos] = CHSV(star[i].color, 255, star[i].brightness);
      leds[star[randomStar].pos] = CHSV(star[randomStar].color, 255, star[randomStar].brightness);
      //Extend life and decrease brightness of star
      star[i].Shine();
      star[randomStar].Shine();
      //Swap positions of these stars
      int temp = star[i].pos;
      star[i].pos = star[randomStar].pos;
      star[randomStar].pos = temp;
    }
  }
  FastLED.show();
  FastLED.clear();
  delay(10);
}

int readInput() {
  int audio = analogRead(MIC_PIN);
  audio -= NOISE;
  audio = abs(audio); //Turns negative value to positive
  return audio;
}
float audioMax(int audio, int multiplier) {
  float average = 0;
  samples[sampleCount] = audio; //Add one new sample value to samples
  sampleCount++;

  for (int i = 0; i < SAMPLES; i++)
    average += samples[i];  // Calculate all samples to average variable

  average /= SAMPLES;   //Divide with amount of SAMPLES to get average of all samples
  MAX_VOL = average * multiplier;   //Higher multiplier increases the sensitivity in most functions
  if (MAX_VOL < MIN_VOL) MAX_VOL = MIN_VOL;
  if (sampleCount >= SAMPLES) sampleCount = 0;

  Serial.println(MAX_VOL);
  return MAX_VOL; //Returning this might seem useless, but I believe it makes the functions more easily readable
}

float fscale(float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve) {

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;

  // condition curve parameter
  // limit range

  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - topLEDtive numbers give more weight to high end on output
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }
  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin) {
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd;
    invFlag = 1;
  }
  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine
  if (originalMin > originalMax ) {
    return 0;
  }
  if (invFlag == 0) {
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;
  }
  else     // invert the ranges
  {
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange);
  }
  return rangedValue;
}
