/*
  ////////////////////////////////////////////////////////////////////////////
  // The Legend of Zelda Interactive Lightbox
  // By Jennifer Diaz
  // 12-14-2017
  // This project is designed for The Legend of Zelda enthusiasts. Place it 
  // in a room or hallway as a decorative and interactive lightbox. It can 
  // also be placed in a dark room and be used as a calming night light, 
  // perfect for kids and kids at heart.
  ////////////////////////////////////////////////////////////////////////////
*/

#include <Adafruit_NeoPixel.h>
#include <Adafruit_VS1053.h>

#define PIXELS_PIN     3
#define NUMPIXELS      8
#define BUTTON_PIN     4

#define VS1053_RESET   -1     // VS1053 reset pin (not used!)
#define VS1053_CS      16     // VS1053 chip select pin (output)
#define VS1053_DCS     15     // VS1053 Data/command select pin (output)
#define CARDCS          2     // Card chip select pin
#define VS1053_DREQ     0     // VS1053 Data request, ideally an Interrupt pin

#define TRIGGEREASTER   10000
#define DELAYBTWSONGS   3000
#define READING_TIMING  5000
#define CHILL_TIMING    70
#define NGTLGT_TIMING   150
#define RAINBOW_TIMING  30
#define EASTER_TIMING   200
#define RESET_TIME      2000
#define DEBOUNCE_TIME   200

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIXELS_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

enum modes {
  START,
  HERO,
  ZELDA,
  NIGHTLIGHT,
  STOP,
  EASTER
};

int value;
int delayval = 300;
int currentMode;
int previousMode;
int baselightValue = 0;
int lastButtonState = 0;
unsigned long lastTimeButtonPressed = 0;

//holding the button
int buttonHoldTime = 0;
int holding = 0;

// Timing Events
unsigned long zeldaEvent = 0;
unsigned long readingLightEvent = 0;
unsigned long chillEvent = 0;
unsigned long nightLightEvent = 0;
unsigned long easterEvent = 0;
unsigned long easterDanceEvent = 0;

// Flags for the first time entering each state
int firstTimeHero = 1;
int firstTimeZelda = 1;
int firstTimeNightlight = 1;
int firstTimeEaster = 1;
int firstOFF = 1;

void setup() {
  Serial.begin(9600);
  pixels.begin();
  pixels.show();

  musicPlayer.begin();
  SD.begin(CARDCS);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);
  musicPlayer.setVolume(40, 40);

  currentMode = START;
}

void loop() {
  switch (currentMode) {
    case START:
      Serial.println("In start mode...");
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 204, 0));
        pixels.show();
        delay(delayval);
      }

      currentMode = HERO;
      break;
    case HERO:
      if (firstTimeHero) {
        Serial.println("In Hero Mode...");
        firstTimeHero = 0;
        musicPlayer.stopPlaying();
        delay(DELAYBTWSONGS);
        musicPlayer.startPlayingFile("01Hero.mp3");
      }

      chillFade();

      if (checkForButtonPress() ) {
        Serial.println("state changing...");
        currentMode = ZELDA;
      }
      break;
    case ZELDA:
      if (firstTimeZelda) {
        Serial.println("In Zelda Mode...");
        firstTimeZelda = 0;
        musicPlayer.stopPlaying();
        delay(DELAYBTWSONGS);
        musicPlayer.setVolume(30, 30);
        musicPlayer.startPlayingFile("26Zelda.mp3");
      }

      rainbowEffect();

      if (checkForButtonPress() ) {
        Serial.println("state changing...");
        currentMode = NIGHTLIGHT;
      }
      break;
    case NIGHTLIGHT:
      if (firstTimeNightlight) {
        Serial.println("In Nightlight Mode...");
        firstTimeNightlight = 0;
        musicPlayer.stopPlaying();
        delay(DELAYBTWSONGS);
        musicPlayer.setVolume(40, 40);
        musicPlayer.startPlayingFile("03Title.mp3");
      }

      slowerChillFade();

      if (checkForButtonPress() ) {
        Serial.println("state changing...");
        firstTimeHero = 1;
        firstTimeZelda = 1;
        firstTimeNightlight = 1;
        firstTimeEaster = 1;
        currentMode = HERO;
      }
      break;
    case STOP:
      if (firstOFF) {
        musicPlayer.stopPlaying();
        for (int pixel = 0; pixel < NUMPIXELS; pixel++) {
          pixels.setPixelColor(pixel, 0, 0, 0);
          pixels.show();
        }
        readingLightEvent = 0;
        firstOFF = 0;
      }

      if (millis() > readingLightEvent) {
        readingLightEvent = millis() + READING_TIMING;
        value = analogRead(A0);
        Serial.print("value: ");
        Serial.println(value);
      }

      if (value > 750) {
        Serial.println("Going to START...");
        firstTimeHero = 1;
        firstTimeZelda = 1;
        firstTimeNightlight = 1;
        firstTimeEaster = 1;
        firstOFF = 1;
        currentMode = START;
      }
      break;

    case EASTER:
      if (firstTimeEaster) {
        musicPlayer.stopPlaying();
        Serial.println("In easter egg mode...");
        delay(DELAYBTWSONGS);
        firstTimeEaster = 0;
      }
      pixelDanceAndSong();
      break;
  }
}

int checkForButtonPress() {
  int buttonState = digitalRead(BUTTON_PIN);

  // track if button is held
  if (millis() > easterEvent) {
    if (buttonState == LOW && !holding) {
      buttonHoldTime = millis();
      holding = 1;
    }

    if (buttonState == HIGH) {
      buttonHoldTime = 0;
      holding = 0;
    }

    if (millis() > RESET_TIME + buttonHoldTime && buttonState == LOW) {
      Serial.println("button is held");
      if (currentMode == 1) {
        previousMode = 3;
      } else if (currentMode == 2) {
        previousMode = 1;
      } else {
        previousMode = 2;
      }

      Serial.print("Leaving mode: ");
      Serial.println(previousMode);
      Serial.println("Changing state b/c of button hold...");
      currentMode = EASTER;
      return 0;
    }
  }

  //track if button is pressed
  if (buttonState == LOW && millis() > lastTimeButtonPressed + DEBOUNCE_TIME &&
      lastButtonState == HIGH) {
    lastTimeButtonPressed = millis();
    //    Serial.println("button is pressed");
    return 1;
  }
  lastButtonState = buttonState;
  return 0;
}

void readingLight() {
  if (millis() > readingLightEvent) {
    readingLightEvent = millis() + READING_TIMING;
    value = analogRead(A0);
    Serial.print("value: ");
    Serial.println(value);
  }

  if (value < 250) {
    Serial.println("Changing states...");
    currentMode = STOP;
  }
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void chillFade() {
  uint16_t pix;
  static uint16_t colors = 0;
  if (millis() > chillEvent) {
    for (pix = 0; pix < NUMPIXELS; pix++) {
      pixels.setPixelColor(pix, Wheel((256 / NUMPIXELS + colors) & 255));
    }
    pixels.show();
    colors++;
    if (colors >= 256) {
      colors = 0;
    }
    chillEvent = millis() + CHILL_TIMING;
  }
}

void slowerChillFade() {
  readingLight();
  uint16_t pix;
  static uint16_t colors = 0;
  if (millis() > nightLightEvent) {
    for (pix = 0; pix < NUMPIXELS; pix++) {
      pixels.setPixelColor(pix, Wheel((256 / NUMPIXELS + colors) & 255));
    }
    pixels.show();
    colors++;
    if (colors >= 256) {
      colors = 0;
    }
    nightLightEvent = millis() + NGTLGT_TIMING;
  }
}

void rainbowEffect() {
  static uint16_t j = 0;

  if (millis() > zeldaEvent) {
    for (uint16_t colors = 0; colors < NUMPIXELS; colors++) {
      pixels.setPixelColor(colors, Wheel(((colors * 256 / NUMPIXELS) + j) & 255));
    }
    pixels.show();
    j++;
    if (j >= 256 * 5) {
      j = 0;
    }
    zeldaEvent = millis() + RAINBOW_TIMING;
  }
}

// EASTER EGG FUNCTIONS
void pixelDanceAndSong() {

  musicPlayer.startPlayingFile("27Menu.mp3");

  while (musicPlayer.playingMusic) {

    if (millis() > easterDanceEvent) {
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(random(255), 255 - random(255), random(255)));
      }
      pixels.show();
      easterDanceEvent = millis() + EASTER_TIMING;
    }
  }

  if (!musicPlayer.playingMusic) {
    easterEvent = millis() + TRIGGEREASTER;
    Serial.print("Going to previousMode: ");

    if (previousMode == 1) {
      firstTimeHero = 0;
      firstTimeZelda = 1;
      firstTimeNightlight = 1;
      firstTimeEaster = 1;
    } else if (previousMode == 2) {
      firstTimeHero = 1;
      firstTimeZelda = 0;
      firstTimeNightlight = 1;
      firstTimeEaster = 1;
    } else if (previousMode == 3) {
      firstTimeHero = 1;
      firstTimeZelda = 1;
      firstTimeEaster = 1;
      firstTimeNightlight = 0;
    }
    Serial.println(previousMode);
    currentMode = previousMode;
  }
}
