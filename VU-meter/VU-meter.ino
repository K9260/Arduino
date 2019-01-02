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
#define MIC_MIN 0
#define MIC_MAX 50
#define NOISE 339
#define SAMPLES 50
#define MIN_VOL 15


CRGB leds[NUM_LEDS];

const int beatSets = 4; //For fast music, 3 or higher. For slow, 1-3
uint8_t hue = 0;
struct beat
{
  int brightness;
  bool exist;
  int center;
  int numBeats; //MUST BE POWER OF 2 AND NOT BIGGER THAN NUM_LEDS
  int stepsTaken;
  uint8_t ghue;
  int maxLength;
  int minLength;

  beat() {
    brightness = 0;
    exist = false;
    center = NUM_LEDS / 2;
    numBeats = NUM_LEDS / 4;
    stepsTaken = center;
    hue = 0;
    maxLength = NUM_LEDS / 2;
    minLength = 2;
  }
  Create(int NumBeats, int Brightness) {
    exist = true;
    numBeats = NumBeats;  //NOTE: numBeats tells only half of the length!
    if (numBeats > maxLength) numBeats = maxLength;
    if (numBeats < minLength) numBeats = minLength;
    if (numBeats % 2 != 0) //numBeats must be power of 2!
      numBeats--;
    center = random(numBeats / 2, NUM_LEDS - (numBeats / 2)); //First beat, others spawn around it
    stepsTaken = center + 1;
    brightness = Brightness;
    ghue = hue + random(25); //EACH BALLSET GETS UNIQUE COLOR
  }
  Move() {
    stepsTaken++;
    if (stepsTaken > NUM_LEDS) //Avoid going over index
      stepsTaken = NUM_LEDS;
    if (stepsTaken * 2 <= numBeats)
      exist = false;

    brightness -= 255 / numBeats;
    if (brightness < 40) { //Politely say no thanks to too dim leds.
      brightness = 0;
      exist = false;
      stepsTaken = center;
    }
  }
};

typedef struct beat Beat;

const int maxBubbles = NUM_LEDS / 2; //Decrease if laggy

struct bubble
{
  int brightness;
  int color;
  int pos;
  int velocity;
  int life;
  int maxLife;
  bool exist;

  bubble() {
    pos = 0;
    velocity = 1; // Increase or decrease if needed, defined for each bubble again in bubbles()
    life = 0;
    maxLife = 80; //How many moves bubble can do
    exist = false;
    brightness = 255;
    color = hue + random(20);
  }
  Move() {
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
    if (life > maxLife || brightness < 10) exist = false;
    if (!exist) {
      brightness = 0;
      life = 0;
    }
  }
  NewColor() {
    color = hue + random(20);
    brightness = 255;
  }
};
typedef struct bubble Bubble;

Beat beat[beatSets];
Bubble bubble[maxBubbles];

int topLED = 0;

int gravityCounter = 0;
int gravity = 2; //HIGHER VALUE EQUALS SLOWER MOVEMENT FOR THE TOP LED
float average = 0;
int averageCounter = 0;
float MAX_VOL = MIC_MAX;
bool reversed = false;
float sensitivity = 0.50; //HIGHER VALUE EQUALS SMALLER CHANCE OF COLOR SWIPE
int index = 0;


void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  Serial.begin(115200);
  pinMode(A0, INPUT);
}
void loop() {

  //vu();
  // dots();
  // split();
  // colorSwipe();
 // beats();
 // bubbles();
   switch (index) {
     case 0: vu();
       break;
     case 1: dots();
       break;
     case 2: split();
       break;
     case 3: colorSwipe();
       break;
     case 4: beats();
       break;
     case 5: bubbles();
       break;
    }
    EVERY_N_SECONDS(30) {
     index++;
     if (index > 5)
       index = 0;
    }
  EVERY_N_SECONDS(10) {
    hue = random(255); //EVERY 10 SECONDS PICK RANDOM COLOR
  }

  FastLED.show();
  FastLED.clear();
  delay(30);
}
void vu() {
  int audio = analogRead(A0); //READING FROM MIC
  audio -= NOISE;
  audio = abs(audio); //TURNS NEGATIVE VALUE TO POSITIVE
  MAX_VOL = audioMax(audio, SAMPLES);

  audio = fscale(MIC_MIN, MAX_VOL, 0, NUM_LEDS - 1, audio, 1.5);

  for (int i = 0; i < audio; i++) {
    leds[i] = CHSV(hue + i * 2, 250, 200 - (i * 2));
  }
  if (audio >= topLED)
    topLED = audio;
  else if (gravityCounter % gravity == 0)
    topLED--;

  leds[topLED] = CHSV(220, 255, 255); //PINK LED AS TOP LED WITH GRAVITY
  if (topLED < NUM_LEDS - 1)            //CHECK TO NOT GO OVER THE INDEX
    leds[topLED + 1] = CRGB(0, 0, 0);   //CLEAR THE PREVIOUS LED

  gravityCounter++;
  if (gravityCounter > gravity)
    gravityCounter = 0;
}

void dots() {
  int audio = analogRead(A0); //READING FROM MIC
  audio -= NOISE;
  audio = abs(audio); //TURNS NEGATIVE VALUE TO POSITIVE
  MAX_VOL = audioMax(audio, SAMPLES);
  audio = fscale(MIC_MIN, MAX_VOL, 1, NUM_LEDS - 1, audio, 1.5); //Scaled from 1 to NUM_LEDS - 1 so top led wont
  //go over index, nor the led below it

  if (audio >= topLED)
    topLED = audio;
  else if (gravityCounter % gravity == 0)
    topLED--;
  leds[topLED] = CHSV(hue, 255, 255); //TOP LED WITH GRAVITY
  leds[topLED - 1] = leds[topLED];    //HIS FRIEND :D

  if (topLED < NUM_LEDS - 1)          //TO NOT GO OVER THE INDEX
    leds[topLED + 1] = CRGB(0, 0, 0);   //CLEAR THE PREVIOUS LED

  gravityCounter++;
  if (gravityCounter > gravity)
    gravityCounter = 0;
}

void split() { //VU METER STARTS FROM THE MIDDLE OF STRIP AND DOES THE SAME EFFECT TO BOTH DIRECTIONS
  //NOTE: NUM_LEDS MUST BE DIVISIBLE BY 2, IF NOT, LEDS MIGHT GO OUT OF INDEX
  int audio = analogRead(A0); //READING FROM MIC
  audio -= NOISE;
  audio = abs(audio); //TURNS NEGATIVE VALUE TO POSITIVE
  MAX_VOL = audioMax(audio, SAMPLES);
  audio = fscale(MIC_MIN, MAX_VOL, NUM_LEDS / 2, NUM_LEDS - 1, audio, 1.0);
  for (int i = NUM_LEDS / 2; i < audio; i++) {
    leds[i] = CHSV(hue + i * 8, 250, 255 + (NUM_LEDS / 2) - i); //LEDS DRAWN UP, BRIGHTNESS GETS LOWER THE HIGHER LED IS DRAWN
    leds[NUM_LEDS - i - 1] = leds[i]; //LEDS DRAWN DOWN
  }
  if (topLED <= audio)
    topLED = audio;
  else if (gravityCounter % gravity * 2 == 0) //GRAVITY MULTIPLIED BY 2 BECAUSE STRIP IS BASICALLY SPLIT INTO TWO PARTS IN THIS
    topLED--;                          //FUNCTION

  leds[topLED] = CHSV(220, 255, 255);         //TOP LED ON TOP OF STRIP (PINK)
  leds[NUM_LEDS - topLED - 1] = leds[topLED]; //ON TOP OF THE BOTTOM HALF OF STRIP

  gravityCounter++;
  if (gravityCounter > gravity * 2)
    gravityCounter = 0;
}
void colorSwipe() { //SWIPES NEW COLOR OVER THE OLD ONE IF VOLUME SPIKES ENOUGH
  int audio = analogRead(A0);
  audio -= NOISE;
  audio = abs(audio); //TURNS NEGATIVE VALUE TO POSITIVE
  MAX_VOL = audioMax(audio, SAMPLES);
  fill_solid(leds, NUM_LEDS, CHSV(hue, 255, 150));
  if (audio > MAX_VOL * sensitivity) {
    hue = random(255);

    if (hue < 127)
      reversed = true;
    else
      reversed = false;

    if (!reversed) {
      for (int i = 0; i < NUM_LEDS; i++) { //DRAW ONE LED AT A TIME (UP)
        leds[i] = CHSV(hue, 255, 150);
        FastLED.show();
        delay(10);
      }
    }
    else {
      for (int i = NUM_LEDS - 1; i >= 0; i--) { //DRAW ONE LED AT A TIME (DOWN)
        leds[i] = CHSV(hue, 255, 150);
        FastLED.show();
        delay(10);
      }
    }
  }
}

void beats() {
  int audio = analogRead(A0);
  audio -= NOISE;
  audio = abs(audio); //TURNS NEGATIVE VALUE TO POSITIVE
  MAX_VOL = audioMax(audio, SAMPLES / 2);
  for (int j = 0; j < beatSets; j++) { //GO THROUGH COLLECTION OF BALL SETS
    if (!beat[j].exist && audio > MAX_VOL * sensitivity * 0.44) {
      //No real explanation for these values, they are found by trial and error
      int brightness = fscale(MIC_MIN, MAX_VOL / 3, 150, 255, audio, 1.5);
      int numBeats = random(4, NUM_LEDS / 3); //Need to be adjusted accordingly with beatSets
      beat[j].Create(numBeats, brightness);
    }
    if (beat[j].exist) {
      for (int i = beat[j].center; i < beat[j].stepsTaken; i++) {
        int temp = beat[j].center * 2 - i - 1;
        if(temp < 0) temp = 0;
        if(i > NUM_LEDS -1) break; //SHOULDNT HAPPEN, but lets just be sure nothing bad happens
        leds[i] = CHSV(beat[j].ghue, 255, beat[j].brightness);
        leds[temp] = leds[i];
      }
      beat[j].Move();
      delay(5);
    }
  }
}
void bubbles() { //Spawns beats that move
  int audio = analogRead(A0);
  audio -= NOISE;
  audio = abs(audio); //TURNS NEGATIVE VALUE TO POSITIVE
  MAX_VOL = audioMax(audio, SAMPLES);
  if (audio > MAX_VOL * sensitivity * 0.7)
  {
    int randomBubble = random(maxBubbles);
    if (bubble[randomBubble].exist == true) {
      bubble[randomBubble].life = 0; //Give that bubble longer life
      bubble[randomBubble].NewColor(); // And new color
    }
    else {
      bubble[randomBubble].exist = true;
      bubble[randomBubble].NewColor();
    }
    if (audio > MAX_VOL)
      bubble[randomBubble].velocity = 2;
    else
      bubble[randomBubble].velocity = 1;
  }
  for (int i = 0; i < maxBubbles; i++) {
    if (bubble[i].exist) {
      leds[bubble[i].pos] = CHSV(bubble[i].color, 255, bubble[i].brightness);
      bubble[i].Move();
    }
  }
}
float audioMax(int audio, int samples) {
  average += audio;
  averageCounter++;
  if (averageCounter > samples) {
    average /= averageCounter;
    MAX_VOL = average * 6;            //HIGHER MULTIPLIER MAKES LEDS STAY LOWER ON AVERAGE
    if (MAX_VOL < MIN_VOL)            //Depending on the function using it ofc
      MAX_VOL = MIN_VOL;

    Serial.println(MAX_VOL);
    average = 0;
    averageCounter = 0;
  }
  return MAX_VOL;
}

float fscale( float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve) {

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
