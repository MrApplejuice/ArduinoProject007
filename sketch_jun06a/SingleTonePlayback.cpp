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
      samples[sampleCount].duration = duration * 8;
      
      sampleCount++;
    }
  }
  preventPlaying = false;
}
    
void BackgroundMusicPlayer :: internalIC() {
  if (millis() >= nextAction) {
    if (playbackCursor < sampleCount) {
      tone(pin, samples[playbackCursor].frequency, samples[playbackCursor].duration);
      noteCounter++;
      
      nextAction = millis() + static_cast<unsigned long>(samples[playbackCursor].duration);
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
      Serial.print("Loaded sample count ");
      Serial.println(sampleCount);
    }
  }

  nextAction = millis();
  
  preventPlaying = false;
}

void BackgroundMusicPlayer :: playSingleToneMusic(const __FlashStringHelper* filename) {
  String fn(filename);
  playSingleToneMusic(fn.c_str());
}

BackgroundMusicPlayer* BackgroundMusicPlayer :: instance(int pin) {
  if (singleton == NULL) {
    singleton = new BackgroundMusicPlayer(pin);
  }
  return singleton;
}



