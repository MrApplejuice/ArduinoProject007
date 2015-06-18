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

// Global execution environment
LCDDisplay* lcd_display;
Numpad* numpad;
ReversedCharset* rCharset;
BackgroundMusicPlayer* musicPlayer;

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

/////////////////////////// "PROGRAMS" //////////////////////////////

class Program {
  private:
  public:
    virtual void switchedTo() {}
    virtual Program* run() = 0;
};

// Default fallback program
class OS : public Program {
  public:
    virtual void switchedTo() {
      musicPlayer->stop();
      tone(DIGITAL_SOUND_PIN, 100, 100);
      
      lcd_display->cls();
      
      rCharset->displayString(0, 10, "NUMPAD OS v1.0", true);
      lcd_display->fillRow(LCDDisplay::ROW_COUNT - 1 - 1, 0, 128, 0x80);
      
      rCharset->displayString(2, 5, "Press # to return to this menu", true);
      rCharset->displayString(3, 5, "at any time.", true);
      rCharset->displayString(5, 5, "(1) Slide show.", true);
    }
  
    virtual Program* run() {
      return this;
    }
};


Program* currentProgram;
OS* osProgram;

void setup() {
  lcd_display = new LCDDisplay;
  lcd_display->cls();
  lcd_display->activateDisplay(true);

  rCharset = new ReversedCharset(lcd_display);
  
  numpad = new Numpad(NUMPAD_START_PIN);

  musicPlayer = BackgroundMusicPlayer::instance(DIGITAL_SOUND_PIN);

  // Debug output initialization
  Serial.begin(9600);

  // SD card initialization
  SD.begin(SPI_PIN);
  
  //displayImage(F("images/img_1.bim"));
  //BackgroundMusicPlayer::instance(DIGITAL_SOUND_PIN)->playSingleToneMusic(F("music/bsno1.nsq"));
  
  osProgram = new OS();

  currentProgram = osProgram;
  currentProgram->switchedTo();
}

void loop() {
  // Cooperative multitasking - currentProgram
  Program* newProgram = currentProgram->run();
  if ((newProgram == NULL) || (numpad->isPressed('#'))) {
    newProgram = osProgram;
  }
  
  if (currentProgram != newProgram) {
    currentProgram = newProgram;
    currentProgram->switchedTo();
  }
  
  // Regular updates...
  BackgroundMusicPlayer::instance(DIGITAL_SOUND_PIN)->updateBuffer();
}


