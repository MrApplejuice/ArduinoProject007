#ifndef REVERSEDCHARSET_H_
#define REVERSEDCHARSET_H_

#include <Arduino.h>

#include <stdint.h>

#include "LCD.h"

class ReversedCharset {
  private:
    static const uint8_t CHARSET_DATA[] PROGMEM;
    static const uint16_t CHARACTER_OFFSETS[] PROGMEM;
    static const int8_t CHAR_LOOKUP_TABLE[] PROGMEM;
    
    LCDDisplay* display;
  public:
    void displayString(int row, int offset, const char* text, bool clearBackground);

    ReversedCharset(LCDDisplay* display);
};

#endif // REVERSEDCHARSET_H_

