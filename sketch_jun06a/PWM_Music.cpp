// somehow this cannot include the required TimerThree library. disabling build of this lib for now
#define EXCLUDE

#ifdef EXCLUDE
#undef EXCLUDE
#else

#include "PWM_Music.h"

#include <stdint.h>
#include <string.h>

#include <Arduino.h>
#include <TimerThree.h>

#include <SD.h>

static PROGMEM const uint8_t MUSIC_PWM_DIVISORS[128] = {0xff, 127, 84, 63, 50, 42, 36, 31, 27, 25, 22, 20, 19, 17, 16, 15, 14, 13, 12, 12, 11, 11, 10, 10, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

static uint8_t MUSIC_PIN = 0;

void setMusicSoundPin(int i) {
  MUSIC_PIN = i;
}

bool playMusic(const char* filename) {
  // Arduino library designers do not know const correctness :-(
  char localFilename[256];
  strncpy(localFilename, filename, 255);
  
  Serial.println(localFilename);
  if (!SD.exists(localFilename)) {
    return false;
  }
  
  Serial.println("File exists!");
  
  File musicFile = SD.open(filename);
  
  Serial.println("Playing file...");
  pinMode(MUSIC_PIN, OUTPUT);
  
  static int BUFFER_SIZE = 8;
  char buffer[BUFFER_SIZE];
  int lastBufferByte = 0;

  while (lastBufferByte < BUFFER_SIZE) {  
    int b = musicFile.read();
    if (b < 0) {
      break;
    }
    
    buffer[lastBufferByte] = b;
    lastBufferByte++;
  }
  
  Timer3.initialize(1);
  while (lastBufferByte > 0) {
    //__c++;
    //if (__c >= 32000) { break; } // noise stop ;-)
    
    uint8_t b = (uint8_t) buffer[0];
    Timer3.pwm(MUSIC_PIN, b * 4);
    unsigned long now = micros();

    for (int i = 1; i < lastBufferByte; i++) {
      buffer[i-1] = buffer[i];
    }
    lastBufferByte--;
    
    while (lastBufferByte < BUFFER_SIZE) {  
      const int readBytes = musicFile.readBytes(buffer + lastBufferByte, BUFFER_SIZE - lastBufferByte);
      lastBufferByte += readBytes;
      if (readBytes < BUFFER_SIZE) {
        break;
      }
    }
    while (micros() - now < 1000 / 8) {}
  }
  digitalWrite(MUSIC_PIN, LOW);
  Serial.println("Done!");
  
  musicFile.close();
}

#endif // EXCLUDE

