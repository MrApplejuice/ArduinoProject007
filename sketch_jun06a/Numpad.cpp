#include "Numpad.h"

bool Numpad :: isPressed(char c) {
  for (int i = 0; i < KEY_COUNT; i++) {
    if (c == KEY_SEQUENCE[i]) {
      return digitalRead(startpin + i) == LOW;
    }
  }
  return false;
}

const char* Numpad :: getPressed() {
  int writeI = 0;
  for (int i = 0; i < KEY_COUNT; i++) {
    if (digitalRead(startpin + i) == LOW) {
      pressedKeys[writeI] = KEY_SEQUENCE[i];
      writeI++;
    }
  }
  pressedKeys[writeI] = '\0';
  return pressedKeys;
}

Numpad :: Numpad(uint8_t startpin) : startpin(startpin) {
  for (int i = startpin; i < startpin + KEY_COUNT; i++) {
    pinMode(i, INPUT_PULLUP);
    Serial.print("sdas ");
    Serial.println(i);
  }
}

const char Numpad::KEY_SEQUENCE[Numpad::KEY_COUNT] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '*', '#'};

