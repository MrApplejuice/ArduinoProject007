#include <stdint.h>

#include "LCD.h"

// Must list ALL used libs here - magic of the arduino software is based on this
#include <SPI.h>
#include <SD.h>
#include <TimerOne.h>

#include "SingleTonePlayback.h"
#include "Numpad.h"
#include "ReversedCharset.h"

const PROGMEM uint8_t DIGITAL_SOUND_PIN = 2;

const PROGMEM uint8_t SPI_PIN = 4; // Required for sd card connection!!!

const PROGMEM uint8_t NUMPAD_START_PIN = 38;


LCDDisplay* lcd_display;
Numpad* numpad;
ReversedCharset* rCharset;

bool displayImage(const char* filename) {
  // Arduino library designers do not know const correctness :-(
  char localFilename[64];
  strncpy(localFilename, filename, 63);

  if (!SD.exists(localFilename)) {
    return false;
  }

  uint8_t rowData[LCDDisplay::DISPLAY_WIDTH];

  File file = SD.open(filename);
  if (file) {
    for (int row = 0; row < LCDDisplay::ROW_COUNT; row++) {
      const int bytesRead = file.readBytes(rowData, LCDDisplay::DISPLAY_WIDTH);
      if (bytesRead < LCDDisplay::DISPLAY_WIDTH) {
        Serial.println(F("Unexpected end of (image) file"));
        break;
      }
      lcd_display->writeRow(row, 0, LCDDisplay::DISPLAY_WIDTH, rowData);
    }
  }
  file.close();
  
  return true;
}

bool displayImage(const __FlashStringHelper* str) {
  String s(str);
  return displayImage(s.c_str());
}

void setup() {
  lcd_display = new LCDDisplay;
  rCharset = new ReversedCharset(lcd_display);
  
  lcd_display->activateDisplay(false);

  // Debug output initialization
  Serial.begin(9600);

  // SD card initialization
  SD.begin(SPI_PIN);
  
  numpad = new Numpad(NUMPAD_START_PIN);
  
  lcd_display->cls();
  lcd_display->activateDisplay(true);
  
  //displayImage(F("images/img_1.bim"));
  rCharset->displayString(0, 1, "0123456789!\"\\/'#*", true);
  rCharset->displayString(1, 1, "Das sieht aber super aus.", true);
  rCharset->displayString(2, 1, "Ganz viel Platz", true);
  rCharset->displayString(3, 1, "fuer Nachrichten", true);
  rCharset->displayString(4, 1, "fuer Mr X... ", true);
  rCharset->displayString(5, 1, "Super gemacht!! ", true);
  rCharset->displayString(7, 1, ":-x :) Thumbs up!", true);
  
  BackgroundMusicPlayer::instance(DIGITAL_SOUND_PIN)->playSingleToneMusic(F("music/bsno1.nsq"));
}

void loop() {
  BackgroundMusicPlayer::instance(DIGITAL_SOUND_PIN)->updateBuffer();
  if (numpad->isPressed('0')) {
    noTone(DIGITAL_SOUND_PIN);
  }
  delay(1);
}
