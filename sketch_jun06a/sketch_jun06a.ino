#include <stdint.h>

#include "LCD.h"

#include <SPI.h>
#include <SD.h>

#include <TimerOne.h>

#include "Tone.h"

const PROGMEM uint8_t DIGITAL_SOUND_PIN = 2;

const PROGMEM uint8_t SPI_PIN = 4; // Required for sd card connection!!!

const PROGMEM int BACKGROUND_MUSIC_TIMER_PIN = 2;

LCDDisplay* lcd_display;

class StreamLineReader {
  private:
    const PROGMEM static size_t BUFFER_SIZE = 64;
  
    Stream* stream;
 
    char buffer[BUFFER_SIZE];
    uint8_t readBufferFillState, lastLineLength;
  public:
    const char* readLine() {
      if (stream) {
        if (lastLineLength > 0) {
          // Shift buffer contents to the beginning
          for (int i = 0; i < readBufferFillState - lastLineLength; i++) {
            buffer[i] = buffer[i + lastLineLength];
          }
          readBufferFillState -= lastLineLength;
        }
      
        if (readBufferFillState < BUFFER_SIZE) {
          readBufferFillState += stream->readBytes(buffer + readBufferFillState, BUFFER_SIZE - readBufferFillState);
        }
        
        // Try to extract a next line (terminate full buffer with \0 if no \n can be found and return that - limits the line length to the BUFFER_SIZE - 1)
        if (readBufferFillState > 0) {
          int _end = 0;
          while ((_end < readBufferFillState - 1) && (buffer[_end] != '\n')) {
            _end++;
          }
          buffer[_end] = '\0';
          lastLineLength = _end + 1;
          return buffer;
        } else {
          return NULL; // Everything has been read
        }
      }
      return NULL;
    }
  
    StreamLineReader() : stream(NULL) {}
    StreamLineReader(Stream& stream) : stream(&stream), readBufferFillState(0), lastLineLength(0) {}
};

class BackgroundMusicPlayer {
  private:
    typedef struct MusicSample {
      int frequency, duration; 
    };
  
    static PROGMEM const int MAX_SAMPLES = 32;
  
    bool preventPlaying;
  
    int nextAction;
    uint8_t pin;
  
    uint8_t sampleCount, playbackCursor;
    MusicSample samples[MAX_SAMPLES];
    
    File openFile;
    StreamLineReader lineReader;
    
    static BackgroundMusicPlayer* singleton;
    
    void loadNext() {
    }
    
    void internalIC() {
      if (millis() >= nextAction) {
        if (playbackCursor < sampleCount) {
          tone(pin, samples[playbackCursor].frequency, samples[playbackCursor].duration);
          nextAction = millis() + samples[playbackCursor].duration;
          playbackCursor++;
        }
      }
    }
    
    static void interruptCallback() {
      if (!singleton->preventPlaying) {
        singleton->internalIC();
      }
    }

    BackgroundMusicPlayer(uint8_t pin) : preventPlaying(false), pin(pin), sampleCount(0), playbackCursor(0) {
      pinMode(pin, OUTPUT);

      Timer1.initialize(100);
      Timer1.attachInterrupt(&interruptCallback);
    }
  public:
    void playSingleToneMusic(const char* filename) {
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
          
          if (openFile) {
            while ((sampleCount < MAX_SAMPLES) && openFile.available()) {
              const char* line = lineReader.readLine();
              if (line == NULL) {
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
        }
      }

      nextAction = millis();
      
      preventPlaying = false;
    }
  
    static BackgroundMusicPlayer* instance(int pin) {
      if (singleton == NULL) {
        singleton = new BackgroundMusicPlayer(pin);
      }
      return singleton;
    }
};
BackgroundMusicPlayer* BackgroundMusicPlayer::singleton = NULL;



void backgroundPlaySingleToneMusic(const char* filename) {
}

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
  // Debug output initialization
  Serial.begin(9600);

  // SD card initialization
  SD.begin(SPI_PIN);
  
  lcd_display = new LCDDisplay;
  
  lcd_display->cls();
  lcd_display->activateDisplay(true);
  
  displayImage(F("images/img_1.bim"));
  
  BackgroundMusicPlayer::instance(DIGITAL_SOUND_PIN)->playSingleToneMusic("music/bsno1.nsq");

  //delay(1000);

  //int freq = 400;

  //displayImage("images/img_30.bim");
}

void loop() {
}
