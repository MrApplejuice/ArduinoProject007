#ifndef NUMPAD_H_
#define NUMPAD_H_

#include <Arduino.h>

class Numpad {
  public:
    static const PROGMEM uint8_t KEY_COUNT = 12;
    static const char KEY_SEQUENCE[KEY_COUNT];
  private:
    uint8_t startpin;
    
    char pressedKeys[12 + 1];
  public:
    bool isPressed(char c);
    
    const char* getPressed();
  
    Numpad(uint8_t startpin);
};



#endif // NUMPAD_H_
