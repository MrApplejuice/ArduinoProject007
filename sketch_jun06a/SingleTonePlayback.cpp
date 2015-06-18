#include "SingleTonePlayback.h"


BackgroundMusicPlayer* BackgroundMusicPlayer::singleton = NULL;

void BackgroundMusicPlayer :: fillBuffer() {
  preventPlaying = true;
  if (openFile) {
    sampleCount = 0;
    playbackCursor = 0;
    while (sampleCount < MAX_SAMPLES) {
      const char* line = lineReader.readLine();
      if (line == NULL) {
        openFile.close();
        break;
      }
      if (strlen(line) == 0) {
        continue;
      }
      
      int freq, duration;
      sscanf(line, "%d,%d", &freq, &duration);
      
      samples[sampleCount].frequency = freq;
      samples[sampleCount].duration = duration * 8; // Magic number to map from MIDI ticks to ms
      
      sampleCount++;
    }
  }
  preventPlaying = false;
}

bool overrunLessThanEquals(unsigned int a, unsigned int b) {
  // Tests a <= b, assuming that b might have overrun 0xFFFFFFFF by a small margin
  if (a > b) {
    if ((b - a) < ((b + 0x7FFFFFFF) - a)) {
      // b did not overflow
      return a <= b;
    } else {
      // b did overflow, a did not (assumption), therefore a < b
      return true;
    }
  } else {
    // always true, if assuming that a cannot overflow
    return true; 
  }
}

void BackgroundMusicPlayer :: internalIC() {
  const unsigned long now = millis();
  if (overrunLessThanEquals(nextAction, now)) {
    if (playbackCursor < sampleCount) {
      tone(pin, samples[playbackCursor].frequency, samples[playbackCursor].duration);
      noteCounter++;
      
      nextAction = now + static_cast<unsigned long>(samples[playbackCursor].duration);
      playbackCursor++;
    }
  }
}

void BackgroundMusicPlayer :: interruptCallback() {
  if (!singleton->preventPlaying) {
    singleton->internalIC();
  }
}

BackgroundMusicPlayer :: BackgroundMusicPlayer(uint8_t pin) : preventPlaying(false), pin(pin), sampleCount(0), playbackCursor(0), noteCounter(0) {
  pinMode(pin, OUTPUT);

  // This does not quite work... will now be done completelty with cooperative multitasking and updateBuffer()
  //Timer1.initialize(1000);
  //Timer1.attachInterrupt(&interruptCallback);
}

void BackgroundMusicPlayer :: updateBuffer() {
  if (playbackCursor >= sampleCount) {
    fillBuffer();
  }
  internalIC();
}
  
void BackgroundMusicPlayer :: playSingleToneMusic(const char* filename) {
  preventPlaying = true;
  
  {
    openFile.close();
    lineReader = StreamLineReader();
    
    sampleCount = 0;
    playbackCursor = 0;
    
    {
      char localFilename[64];
      strncpy(localFilename, filename, 63);
      openFile = SD.open(localFilename);
    }
    if (!openFile) {
      Serial.println("Cannot open music file");
    } else {
      lineReader = StreamLineReader(openFile);
      fillBuffer();      
    }
  }

  nextAction = millis();
  
  preventPlaying = false;
}

void BackgroundMusicPlayer :: playSingleToneMusic(const __FlashStringHelper* filename) {
  String fn(filename);
  playSingleToneMusic(fn.c_str());
}

void BackgroundMusicPlayer :: stop() {
  preventPlaying = true;
  
  sampleCount = 0;
  playbackCursor = 0;
  openFile.close();
  
  noTone(pin);
  
  preventPlaying = false;
}

bool BackgroundMusicPlayer :: isPlaying() const {
  return playbackCursor < sampleCount; // only works in cooperative mode :-(
}

BackgroundMusicPlayer* BackgroundMusicPlayer :: instance(int pin) {
  if (singleton == NULL) {
    singleton = new BackgroundMusicPlayer(pin);
  }
  return singleton;
}



