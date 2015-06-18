#ifndef SINGLETONEPLAYBACK_H_
#define SINGLETONEPLAYBACK_H_

#include <Arduino.h>

#include <SD.h>

class StreamLineReader {
  private:
    const PROGMEM static size_t BUFFER_SIZE = 48;
  
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
          if (stream->available()) {
            readBufferFillState += stream->readBytes(buffer + readBufferFillState, BUFFER_SIZE - readBufferFillState);
          }
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
  
    static PROGMEM const int MAX_SAMPLES = 4;
  
    bool preventPlaying;
  
    unsigned long nextAction;
    uint8_t pin;
  
    uint8_t sampleCount, playbackCursor;
    MusicSample samples[MAX_SAMPLES];
    
    File openFile;
    StreamLineReader lineReader;
    
    static BackgroundMusicPlayer* singleton;
    
    void fillBuffer();    
    void internalIC();
    static void interruptCallback();

    BackgroundMusicPlayer(uint8_t pin);
  public:
    int noteCounter;

    void updateBuffer();
  
    void playSingleToneMusic(const char* filename);
    void playSingleToneMusic(const __FlashStringHelper* filename);
    
    void stop();
    
    bool isPlaying() const;
  
    static BackgroundMusicPlayer* instance(int pin);
};

#endif // SINGLETONEPLAYBACK_H_

