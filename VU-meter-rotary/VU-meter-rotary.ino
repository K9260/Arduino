/*
   VU meter with rotary encoder and LCD display
   By Reko Meriö
   https://github.com/rekomerio
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <FastLED.h>

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

/*** DATA PINS ***/
#define ROTARY_CLK     2
#define ROTARY_DT      3
#define BUTTON_PIN     4
#define DATA_PIN0      6
#define DATA_PIN1      7
#define MIC_PIN        A0
/*** --------- ***/
#define LED_TYPE       WS2812
#define COLOR_ORDER    GRB
#define NUM_LEDS       72
#define MIC_MIN        0
#define NOISE          339
#define MIN_VOL        15
#define MIN_LEDS       14
#define SAMPLING_T     2     /* Dont put higher value than 3, or then change the delays */
#define BLOCK_INTERVAL 300   /* How long to wait before next block movement can be done(milliseconds) */
/*** --------- ***/
#define MAXPARTICLES   25    /* NOTE: Nothing below can be higher than maxparticles... */
#define MAXBUBBLES     20    /* ...as bubbles, ripples etc all use particle struct!    */
#define MAXBEATS       10    /* Value has to be divisible by two */
#define MAXRIPPLES     6     /* Value has to be divisible by two */
#define MAXTRAILS      4
#define MAXBLOCKS      4


CRGB leds[NUM_LEDS];
LiquidCrystal_I2C lcd(0x27, 16, 2);

uint8_t hue = 0;
uint8_t blockCount = 0;

struct block
{
  uint8_t pos[NUM_LEDS / 8]; //Array of led positions in the block
  uint8_t startPoint; //Where the block will start

  block() {
    init();
    blockCount++;
  }
  void moveUp() {
    for (int i = 0; i < NUM_LEDS / 8; i++) {
      pos[i]++;
    }
  }
  void moveDown() {
    for (int i = 0; i < NUM_LEDS / 8; i++) {
      pos[i]--;
    }
  }
  void init() {
    if (blockCount > 0) {
      startPoint = (NUM_LEDS / 8) * blockCount * 2;
    }
    else {
      startPoint = 0;
    }
    for (int i = 0; i < NUM_LEDS / 8; i++) {
      pos[i] = startPoint + i;
    }
  }
};
typedef struct block Block;

struct particle
{
  uint8_t brightness;
  uint8_t color;
  uint8_t maxVelocity;
  uint16_t life;
  uint16_t maxLife;
  float pos;
  float velocity;
  bool exist;

  particle() {
    init();
  }

  void firework() {
    pos += velocity;
    velocity *= 0.95;
    brightness -= 2;
    if (pos >= NUM_LEDS - 1) {
      pos = NUM_LEDS - 1;
    }
    if (pos <= 0) {
      pos = 0;
    }
    if (brightness < 10) {
      exist = false;
      brightness = 0;
    }
  }

  void bubble() {
    if (velocity > maxVelocity) {
      velocity = maxVelocity;
    }
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
    if (life > maxLife || brightness < 20) {
      initBubble();
    }
  }

  void ripple() {
    pos += velocity;
    life++;
    if (pos > NUM_LEDS - 1) {
      pos = NUM_LEDS - 1;
      exist = false;
    }
    if (pos < 0) {
      pos = 0;
      exist = false;
    }
    brightness -= (255 / maxLife);
    if (life > maxLife || brightness < 20) {
      initRipple(15);
    }
  }

  void init() {
    pos        = 0;
    velocity   = 0;
    brightness = 0;
    life       = 0;
    maxLife    = 0;
    color      = hue;
    exist      = false;
  }

  void initFirework(uint8_t Pos) {
    pos        = Pos;
    velocity   = random(5, 20);
    velocity  /= 10; // Velocity is between 0.5 - 1.9
    exist      = true;
    brightness = random(200, 256);
    color      = hue + random(30);
  }

  void initBubble() {
    pos         = random(0, NUM_LEDS);
    velocity    = 0.5 + (random(30) * 0.01); // Increase or decrease if needed
    life        = 0;
    maxLife     = 90; //How many moves bubble can do
    exist       = false;
    brightness  = 255;
    color       = hue + random(30);
    maxVelocity = 2;
  }

  void initRipple(uint8_t MaxLife) {
    pos        = random(MaxLife, NUM_LEDS - MaxLife); //Avoid spawning too close to edge
    velocity   = 1;
    life       = 0;
    maxLife    = MaxLife;
    exist      = false;
    brightness = 255;
    color      = hue;
  }

  void newColor() {
    color = hue + random(20);
    brightness = 255;
  }
};
typedef struct particle Particle;

Block block[MAXBLOCKS];
Particle particle[MAXPARTICLES];

int8_t sensitivity      = 20;
uint8_t program         = 0;      //Index of running program
uint8_t buttonState     = 0;
uint8_t buttonStatePrev = 0;
uint8_t blockMoves      = 0;
uint8_t block_dir;

uint16_t audio;

uint32_t lastTime = 0;

float gravity           = 0.55;   //Higher value = leds fall back down faster
float topLED            = 0;

bool changeColor        = false;  //Does the function randomly change color every n seconds
bool permission_to_move = false;
bool display;

enum BlockSize { one_block, two_blocks, four_blocks };
BlockSize blocksize = four_blocks;

void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN0, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, DATA_PIN1, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip); //Currently running two strips
  Serial.begin(115200);
  pinMode(MIC_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(ROTARY_CLK, INPUT);
  pinMode(ROTARY_DT, INPUT);

  attachInterrupt(digitalPinToInterrupt(ROTARY_CLK), updateEncoder, CHANGE);

  lcd.init();
  lcd.clear();
}

void (*modes[])() = {
  &blank,
  &vu,
  &dots,
  &split,
  &brush,
  &beats,
  &bubbles,
  &ripples,
  &trails,
  &blocks,
  &fireworks
};

#define MODES_LENGTH sizeof(modes) / sizeof(modes[0])

void loop() {
  audio = sample(SAMPLING_T);

  if (buttonPressed()) {
    program++;
    program %= MODES_LENGTH;
    drawDisplay();
  }
  if (program) {
    EVERY_N_MILLISECONDS(1000) {
      drawDisplay();
    }
  }
  if (changeColor) {
    EVERY_N_SECONDS(10) {
      hue = random(256); //Every 10 seconds pick random color
    }
  }
  modes[program]();
}

bool buttonPressed() {
  bool pressed;
  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState && buttonState != buttonStatePrev) {
    pressed = true;
  } else {
    pressed = false;
  }
  buttonStatePrev = buttonState;
  return pressed;
}

void updateEncoder() {
  uint8_t k0 = digitalRead(ROTARY_CLK);
  uint8_t k1 = digitalRead(ROTARY_DT);
  if (k0 != k1) {
    sensitivity++;
  } else {
    sensitivity--;
  }
  sensitivity = constrain(sensitivity, 0, 100);
  delay(1); // Weird jumpy updates are happening without any delay
}

void drawDisplay() {
  lcd.clear();
  if (!display) {
    lcd.display();
    lcd.backlight();
    display = true;
  }
  /** 1st row **/
  lcd.setCursor(0, 0);
  lcd.print("SENS:");
  lcd.print(sensitivity);
  lcd.print(" VOL:");
  lcd.print(audio);
  /** 2nd row **/
  lcd.setCursor(0, 1);
  lcd.print("MODE:");
  lcd.print(program);
  lcd.print(" HUE:");
  lcd.print(hue);
}

void vu() {
  changeColor = true;
  audio = fscale(MIC_MIN, sensitivity, 0, NUM_LEDS - 1, audio, 0.5);
  if (audio < MIN_LEDS && sensitivity <= MIN_VOL) {
    audio = 0; // Avoid flickering caused by small interference
  }
  for (uint8_t i = 0; i < audio; i++) {
    leds[i] = CHSV(hue + i * 2, 255, 255);
  }

  topLED = (topLED >= audio) ? topLED - gravity : audio;

  if (topLED < 0) {
    topLED = 0;
  }

  leds[(uint8_t)topLED] = CHSV(220, 255, 255); //Pink led as top led with gravity
  FastLED.show();
  fadeToBlackBy(leds, NUM_LEDS, 85); //Looks smoother than using FastLED.clear()
  delay(5 - SAMPLING_T);
}

void dots() {
  changeColor = true;
  audio = fscale(MIC_MIN, sensitivity, 1, NUM_LEDS - 1, audio, 0.5);

  topLED = (topLED >= audio) ? topLED - gravity : audio;

  if (topLED < 1) {
    topLED = 1;
  }

  leds[(uint8_t)topLED] = CHSV(hue, 255, 255);      //Top led with gravity
  leds[(uint8_t)topLED - 1] = leds[(uint8_t)topLED];    //His friend :D

  FastLED.show();
  FastLED.clear();
  delay(5 - SAMPLING_T);
}

void split() { //Vu meter starting from middle of strip and doing same effect to both directions
  changeColor = true;
  audio = fscale(MIC_MIN, sensitivity, NUM_LEDS / 2, NUM_LEDS - 1, audio, 1.0);
  if (audio < MIN_LEDS / 2 + (NUM_LEDS / 2) && sensitivity <= MIN_VOL) {
    audio = NUM_LEDS / 2; //Avoid flickering caused by small interference
  }
  for (uint8_t i = NUM_LEDS / 2; i < audio; i++) {
    leds[i] = CHSV(hue + i * 6, 250, 255);
    leds[NUM_LEDS - i - 1] = leds[i]; //LED's drawn down
  }

  topLED = (topLED >= audio) ? topLED - (gravity * 0.7) : audio;

  if (topLED < NUM_LEDS / 2) {
    topLED = NUM_LEDS / 2;
  }

  leds[(uint8_t)topLED] = CHSV(220, 255, 255);                  //Top led on top of strip (PINK)
  leds[NUM_LEDS - (uint8_t)topLED - 1] = leds[(uint8_t)topLED]; //On bottom of bottom half of strip

  FastLED.show();
  fadeToBlackBy(leds, NUM_LEDS, 85); //Looks smoother than using FastLED.clear()
  delay(5 - SAMPLING_T);
}

void brush() { // Swipes/brushes new color over the old one
  bool reversed;
  changeColor = false;
  fill_solid(leds, NUM_LEDS, CHSV(hue, 255, 255));
  if (audio > sensitivity * 0.7) {
    hue = random(256);
    reversed = (hue < 127);
    if (!reversed) {
      for (uint8_t i = 0; i < NUM_LEDS; i++) { //Draw one led at a time (UP)
        leds[i] = CHSV(hue, 255, 255);
        FastLED.show();
        delay(5);
      }
    }
    else {
      for (uint8_t i = NUM_LEDS - 1; i > 0; i--) { //Draw one led at a time (DOWN)
        leds[i] = CHSV(hue, 255, 255);
        FastLED.show();
        delay(5);
      }
    }
  }
  FastLED.show();
  FastLED.clear();
}
void beats() {
  changeColor = true;
  EVERY_N_MILLISECONDS(30 - SAMPLING_T) {
    uint8_t newBeats = fscale(MIC_MIN, sensitivity, 2, MAXBEATS, audio, 0.5); //Max amount of beats to be spawned this loop
    if (newBeats % 2 != 0) { //Must be divisible by two
      newBeats--;
    }
    for (int i = 0; i < newBeats; i += 2) {
      if (audio > sensitivity * 0.3 && !particle[i].exist) {
        uint8_t beatlife = random(6, 10);   //Longer life = longer length
        particle[i].initRipple(beatlife);   //initialize life
        particle[i].exist = true;
        particle[i].color += random(30);
        particle[i + 1] = particle[i];      //Everything except velocity is the same for other beats twin
        particle[i + 1].velocity *= -1;     //because we want the other to go opposite direction
      }
    }
    for (uint8_t i = 0; i < MAXBEATS; i++) {
      if (particle[i].exist) {
        leds[(uint8_t)particle[i].pos] = CHSV(particle[i].color, 255, particle[i].brightness);
        particle[i].ripple();
      }
    }
    FastLED.show();
    fadeToBlackBy(leds, NUM_LEDS, 255 / 5);
  }
}

void bubbles() {                         // Spawns bubbles that move when audio peaks enough
  changeColor = true;
  if (audio > sensitivity * 0.3) {
    uint8_t randomBubble = random(MAXBUBBLES);
    if (particle[randomBubble].exist) {
      particle[randomBubble].life /= 2;  // Give that bubble longer life
      particle[randomBubble].newColor(); // And new color
    } else {
      particle[randomBubble].exist = true;
      particle[randomBubble].newColor();
    }
  }
  for (uint8_t i = 0; i < MAXBUBBLES; i++) {
    if (particle[i].exist) {
      leds[(uint8_t)particle[i].pos] = CHSV(particle[i].color, 255, particle[i].brightness);
      particle[i].bubble();
    }
  }
  FastLED.show();
  FastLED.clear();
  delay(3 - SAMPLING_T);
}

void ripples() {
  changeColor = false;
  EVERY_N_MILLISECONDS(17  - SAMPLING_T) {
    uint8_t newRipples = fscale(MIC_MIN, sensitivity, 2, MAXRIPPLES, audio, 0.5); //Max amount of ripples to be spawned this loop
    if (newRipples % 2 != 0) { //Must be divisible by two
      newRipples--;
    }
    EVERY_N_MILLISECONDS(200) {
      hue++; //Slight color rotation
    }
    uint8_t b = beatsin8(30, 70, 90);              //Pulsing brightness for background (pulses,min,max)
    fill_solid(leds, NUM_LEDS, CHSV(hue, 255, b)); //Delete this line if you dont like background color
    for (uint8_t i = 0; i < newRipples; i += 2) {
      if (audio > sensitivity * 0.3 && !particle[i].exist) {
        particle[i].initRipple(12);               //initiliaze in case the color has changed after last init
        particle[i].exist = true;
        particle[i + 1] = particle[i];            //Everything except velocity is the same for other ripples twin
        particle[i + 1].velocity *= -1;           //because we want the other to go opposite direction
      }
    }
    for (uint8_t i = 0; i < MAXRIPPLES; i++) {
      if (particle[i].exist) {
        leds[(uint8_t)particle[i].pos] = CHSV(particle[i].color + 30, 180, particle[i].brightness);
        particle[i].ripple();
      }
    }
    FastLED.show();
    FastLED.clear();
  }
}
void trails() { //Spawns trails that move
  changeColor = true;
  if (audio > sensitivity * 0.3) {
    uint8_t randomTrail = random(MAXTRAILS);
    if (particle[randomTrail].exist) {
      particle[randomTrail].life /= 2;          //Extend life of trail
      particle[randomTrail].newColor();         // And give new color
    } else {
      particle[randomTrail].exist = true;
      particle[randomTrail].maxLife = 60;
    }
  }
  for (uint8_t i = 0; i < MAXTRAILS; i++) {
    if (particle[i].exist) {
      leds[(uint8_t)particle[i].pos] = CHSV(particle[i].color, 255, particle[i].brightness);
      particle[i].bubble();
    }
  }
  FastLED.show();
  fadeToBlackBy(leds, NUM_LEDS, 255 / 6);
  delay(3 - SAMPLING_T);
}

void blocks() {
  uint8_t movement;
  changeColor = false;
  if (sensitivity * 0.6 < audio && (uint32_t)(millis() - lastTime) > BLOCK_INTERVAL) {
    permission_to_move = true;
    lastTime = millis();
  }
  if (permission_to_move) {
    switch (blocksize) {
      case one_block:
        two_blocks_open();
        break;
      case two_blocks:
        if (!blockMoves)              //To not change direction of block during the transition
          block_dir = random(2);      //Randomize - if to close into one block, or open into four
        if (block_dir == 0)
          four_blocks_open();
        else
          two_blocks_close();
        break;
      case four_blocks:
        four_blocks_close();
        break;
    }
    delay(10  - SAMPLING_T);
  }
  uint8_t bounce = beatsin8(60, 0, NUM_LEDS / 8); // Bounces all leds up and down in circular motion
  for (uint8_t j = 0; j < MAXBLOCKS; j++) {
    for (uint8_t i = 0; i < NUM_LEDS / 8; i++) {
      leds[block[j].pos[i] + bounce] = CHSV(hue, 255, 255);
    }
  }
  FastLED.show();
  FastLED.clear();
}

void four_blocks_close() {          // Four blocks close into two blocks
  for (uint8_t i = 0; i < MAXBLOCKS; i++) {
    if (i % (MAXBLOCKS / 2) == 0)
      block[i].moveUp();
    else
      block[i].moveDown();
  }
  blockMoves++;
  if (blockMoves > NUM_LEDS / 16) {
    blocksize = two_blocks;
    permission_to_move = false;
    hue += 20;
    blockMoves = 0;
  }
}

void two_blocks_close() {         // Two blocks close into one
  for (uint8_t i = 0; i < MAXBLOCKS; i++) {
    if (i < (MAXBLOCKS / 2))
      block[i].moveUp();
    else
      block[i].moveDown();
  }
  blockMoves++;
  if (blockMoves > NUM_LEDS / 8) {
    blocksize = one_block;
    permission_to_move = false;
    hue = random(256);
    blockMoves = 0;
  }
}

void two_blocks_open() {          // One block opens into two
  for (uint8_t i = 0; i < MAXBLOCKS; i++) {
    if (i < (MAXBLOCKS / 2))
      block[i].moveDown();
    else
      block[i].moveUp();
  }
  blockMoves++;
  if (blockMoves > NUM_LEDS / 8) {
    blocksize = two_blocks;
    permission_to_move = false;
    blockMoves = 0;
  }
}

void four_blocks_open() {         // Two blocks open into four
  for (uint8_t i = 0; i < MAXBLOCKS; i++) {
    if (i % (MAXBLOCKS / 2) == 0)
      block[i].moveDown();
    else
      block[i].moveUp();
  }
  blockMoves++;
  if (blockMoves > NUM_LEDS / 16) {
    blocksize = four_blocks;
    permission_to_move = false;
    blockMoves = 0;
  }
}

void fireworks() {
  changeColor = true;
  bool particles = false;
  if (audio > sensitivity * 0.5) {
    uint8_t newParticles = random(5, MAXPARTICLES + 1);
    for (uint8_t i = 0; i < newParticles; i++) {
      if (particle[i].exist) {    // Check if any of the particles still exist
        particles = true;
      }
    }
    if (!particles) {             // If no particles exist, create new ones
      uint8_t startingPoint = random(NUM_LEDS / 2 - 10, NUM_LEDS / 2 + 10);
      for (uint8_t i = 0; i < newParticles; i++) {
        particle[i].initFirework(startingPoint);
        if (i % 2 == 0) {         // Half of the particles will go up and other half down
          particle[i].velocity *= -1;
        }
      }
    }
  }
  for (uint8_t i = 0; i < MAXPARTICLES; i++) {
    if (particle[i].exist) {      // Update particle if it exists
      leds[uint8_t(particle[i].pos)] = CHSV(particle[i].color, 255, particle[i].brightness);
      particle[i].firework();     // Moves the particle and reduces brightness
    }
  }
  FastLED.show();
  FastLED.clear();
  delay(5 - SAMPLING_T);
}

void blank() {
  if (display) {
    lcd.noDisplay();
    lcd.noBacklight();
    display = false;
  }
  FastLED.clear();
  FastLED.show();
  delay(5);
}

uint16_t readInput() {
  int16_t audio = analogRead(MIC_PIN) - NOISE;
  return abs(audio);
}

uint16_t sample(uint8_t samplingTime) {
  uint16_t samples = 0;
  uint16_t average = 0;
  uint32_t startTime = millis();

  while ((uint32_t)(millis() - startTime) < samplingTime) {
    samples++;
    average += readInput();
  }
  average /= samples;
  return average;
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
