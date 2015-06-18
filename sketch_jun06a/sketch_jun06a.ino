#include <stdint.h>

#include "LCD.h"

// Must list ALL used libs here - magic of the arduino software is based on this
#include <SPI.h>
#include <SD.h>
#include <TimerOne.h>

#include "SingleTonePlayback.h"
#include "Numpad.h"
#include "ReversedCharset.h"

const PROGMEM int KEY_REPEAT_DURATION = 500; // ms - time within which no new key presses should be processed after initial stroke detection - repeat limiter and stroke noise removal

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
    Serial.println(F("Image file does not exist"));
    return false;
  }

  uint8_t rowData[LCDDisplay::DISPLAY_WIDTH];

  File file = SD.open(filename);
  if (!file) {
    Serial.println(F("Could not open file"));
    return false;
  } else {
    for (int row = 0; row < LCDDisplay::ROW_COUNT; row++) {
      const int bytesRead = file.readBytes(rowData, LCDDisplay::DISPLAY_WIDTH);
      if (bytesRead < LCDDisplay::DISPLAY_WIDTH) {
        Serial.println(F("Unexpected end of (image) file"));
        file.close();
        return false;
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

class SlideShow : public Program {
  private:
    unsigned int lastKeyPress;
    
    unsigned long lastImageOrTextTime, lastMusicEndTime;
    
    int imageFiles;
    int textFiles;
    int musicFiles;
    
    int lastImageOrText, currentImageOrText;
    
    int countFiles(String dir) {
      File d = SD.open(dir);
      int result = 0;
      
      if (d) {
        File file = d.openNextFile();
        while (file) {
          if (!file.isDirectory()) {
            result++;
          }
          file.close();
          file = d.openNextFile();
        }
        d.rewindDirectory();
        d.close();
      }
      return result;
    }
    
    String getNthFileName(String dir, int n) {
      File d = SD.open(dir);
      
      if (d) {
        File file = d.openNextFile();
        while (file) {
          String name = file.name();
          bool isDir = file.isDirectory();
          file.close();
          
          if (!isDir) {
            if (n == 0) {
              d.rewindDirectory();
              d.close();
              return dir + String(F("/")) + name;
            }
            
            n--;
          }
          file = d.openNextFile();
        }
        d.rewindDirectory();
        d.close();
      }
      return String();
    }
    
    void playRandomMusic() {
      int i = random(musicFiles);
      String filename = getNthFileName(String(F("/music")), i);

      Serial.print(F("Playing music number "));
      Serial.print(i);
      Serial.print(F(": "));
      Serial.println(filename);

      musicPlayer->playSingleToneMusic(filename.c_str());
      lastMusicEndTime = millis();
    }
    
    void showText(int i) {
      String filename = getNthFileName(String(F("/texts")), i);
      Serial.print(F("Showing text number "));
      Serial.print(i);
      Serial.print(F(": "));
      Serial.println(filename);
      
      File file = SD.open(filename);
      StreamLineReader lineReader(file); // This one comes from SingleTonePlayback.h - quite dirty, but I got no time :-(
      
      lcd_display->cls();
      for (int row = 0; row < LCDDisplay::ROW_COUNT; row++) {
        const char* line = lineReader.readLine();
        if (line == NULL) {
          break;
        }
        rCharset->displayString(row, 0, line, true);
      }
      
      file.close();
    }
    
    void showImage(int i) {
      String filename = getNthFileName(String(F("/images")), i);
      Serial.print(F("Showing image number "));
      Serial.print(i);
      Serial.print(F(": "));
      Serial.println(filename);
      
      bool succeeded = displayImage(filename.c_str());
      if (!succeeded) {
        Serial.println("Display of image failed!");
      }
    }
    
    void showI(int i) {
      lastImageOrText = currentImageOrText;
      currentImageOrText = i;
      
      if (i >= imageFiles) {
        showText(i - imageFiles);
      } else {
        showImage(i);
      }
      
      lastImageOrTextTime = millis();
    }
    
    void nextRandomImageOrText() {
      int next = currentImageOrText;
      while (next == currentImageOrText) {
        next = random(imageFiles + textFiles);
      }
      showI(next);
    }
  public:
    virtual void switchedTo() {
      randomSeed(millis());
      
      lastKeyPress = millis();
      lcd_display->cls();
      
      Serial.println("Counting images");
      imageFiles = countFiles(String(F("/images")));
      Serial.println("Counting texts");
      textFiles = countFiles(String(F("/texts")));
      Serial.println("Counting music");
      musicFiles = countFiles(String(F("/music")));
      
      currentImageOrText = random(imageFiles + textFiles);
      lastImageOrText = currentImageOrText;
      showI(currentImageOrText);
      
      playRandomMusic();
    }

    virtual Program* run() {
      if (millis() - lastKeyPress > KEY_REPEAT_DURATION) {
        // New key presses are allowed now
        if (numpad->isPressed('6')) {
          nextRandomImageOrText();
          lastKeyPress = millis();
        }
        if (numpad->isPressed('4')) {
          showI(lastImageOrText);
          lastKeyPress = millis();
        }
      }
      
      if (millis() - lastImageOrTextTime > 60000) { // 1 min per image or text
        nextRandomImageOrText();
        Serial.print(millis());
        Serial.print("  ");
        Serial.println(lastImageOrTextTime);
      }
      
      if (musicPlayer->isPlaying()) {
        lastMusicEndTime = millis();
      } else if (millis() - lastMusicEndTime > 15000) { // 15 sec max between the start of last note of the last song and the start of the first note of the next song
        playRandomMusic();
      }
      
      return this;
    }
};

class SoundKeyboard : public Program {
  public:
    virtual void switchedTo() {
      musicPlayer->stop();

      lcd_display->cls();
      
      rCharset->displayString(0, 0, "SUPER AWESOME KEYBOARD", true);
      lcd_display->fillRow(LCDDisplay::ROW_COUNT - 1 - 1, 0, 128, 0x80);
      
      rCharset->displayString(2, 5, "1 = A; 2 = A#, 3 = H, 4 = C", true);
      rCharset->displayString(3, 5, "5 = C#; 6 = D; 7 = D#; 8 = E.", true);
      rCharset->displayString(5, 5, "9 = F; * = F#; 0 = G", true);
    }
    
    virtual Program* run() {
      if (numpad->isPressed('1')) {
        tone(DIGITAL_SOUND_PIN, 440);
      } else if (numpad->isPressed('2')) {
        tone(DIGITAL_SOUND_PIN, 466 );
      } else if (numpad->isPressed('3')) {
        tone(DIGITAL_SOUND_PIN, 493 );
      } else if (numpad->isPressed('4')) {
        tone(DIGITAL_SOUND_PIN, 523 );
      } else if (numpad->isPressed('5')) {
        tone(DIGITAL_SOUND_PIN, 554 );
      } else if (numpad->isPressed('6')) {
        tone(DIGITAL_SOUND_PIN, 587 );
      } else if (numpad->isPressed('7')) {
        tone(DIGITAL_SOUND_PIN, 622 );
      } else if (numpad->isPressed('8')) {
        tone(DIGITAL_SOUND_PIN, 659 );
      } else if (numpad->isPressed('9')) {
        tone(DIGITAL_SOUND_PIN, 698 );
      } else if (numpad->isPressed('*')) {
        tone(DIGITAL_SOUND_PIN, 739 );
      } else if (numpad->isPressed('0')) {
        tone(DIGITAL_SOUND_PIN, 783 );
      } else {
        noTone(DIGITAL_SOUND_PIN);
      }
      
      return this;
    }
};

// Default fallback program
class OS : public Program {
  private:
    SlideShow* slideShow;
    SoundKeyboard* keyboard;
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
      rCharset->displayString(6, 5, "(2) Keyboard.", true);
    }
  
    virtual Program* run() {
      if (numpad->isPressed('1')) {
        return slideShow;
      }
      if (numpad->isPressed('2')) {
        return keyboard;
      }
      
      return this;
    }
    
    OS() {
      slideShow = new SlideShow();
      keyboard = new SoundKeyboard();
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


