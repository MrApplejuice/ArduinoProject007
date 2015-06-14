#include "LCD.h"

#include <Arduino.h>

void LCDDisplay :: switchReadMode(ReadWriteMode mode) {
  switch (mode) {
    case READ:
      {
        // FIRST switch all bus pins to read mode, then tell it the LCD
        for (int i = BUS_START_PIN; i <= BUS_END_PIN; i++) {
          pinMode(i, INPUT);
        }
        digitalWrite(WRITE_READ_SWITCH_PIN, HIGH);
      } break;
    case WRITE:
      {
        // Now FIRST tell the LCD that we want to write, then switch bus mode
        digitalWrite(WRITE_READ_SWITCH_PIN, LOW);
        for (int i = BUS_START_PIN; i <= BUS_END_PIN; i++) {
          pinMode(i, OUTPUT);
        }
      } break;
  }
}

void LCDDisplay :: issueCommand() {
  // Pulses ACTIVATE_COMMAND_PIN, executing a command on the LCD
  digitalWrite(ACTIVATE_COMMAND_PIN, LOW);
  digitalWrite(ACTIVATE_COMMAND_PIN, HIGH);
}

void LCDDisplay :: updateState() {
  switchReadMode(READ);
  digitalWrite(COMMAND_MEM_SWITCH_PIN, LOW);

  lastState.displayOn = digitalRead(BUS_START_PIN + 5) == LOW;
  lastState.resetting = digitalRead(BUS_START_PIN + 4) == HIGH;
  lastState.busy = digitalRead(BUS_START_PIN + 7) == HIGH;
}

void LCDDisplay :: busWrite(uint8_t data) {
  switchReadMode(WRITE);
  for (int i = 0; i < 8; i++) {
    digitalWrite(BUS_START_PIN + i, (data >> i) & 1 != 0 ? HIGH : LOW);
  }
}

void LCDDisplay :: activateChip(bool left, bool right) {
  // When switching to another chip, make sure we read something in, 
  // otherwise we might issue involuntary commands on the chip
  // when it is being selected
  updateState();
  
  digitalWrite(CHIP1_SELECT_PIN, left  ? HIGH : LOW);
  digitalWrite(CHIP2_SELECT_PIN, right ? HIGH : LOW);
}

void LCDDisplay :: setICCursorPosition(unsigned int row, unsigned int x) {
  if (row >= ROW_COUNT) {
    row = ROW_COUNT - 1;
  }
  
  if (x >= IC_ROW_WIDTH) {
    x = IC_ROW_WIDTH - 1;
  }
  
  digitalWrite(COMMAND_MEM_SWITCH_PIN, LOW);
  uint8_t PAGE_SELECT_CMD = 0xB8;
  busWrite(PAGE_SELECT_CMD | row);
  issueCommand();
  
  digitalWrite(COMMAND_MEM_SWITCH_PIN, LOW);
  uint8_t ADDRESS_SELECT_CMD = 0x40;
  busWrite(ADDRESS_SELECT_CMD | x);
  issueCommand();
}

void LCDDisplay :: setCursorPosition(unsigned int row, unsigned int x) {
  if (x >= DISPLAY_WIDTH) {
    x = DISPLAY_WIDTH / 2 - 1;
  }
  
  if (x >= IC_ROW_WIDTH) {
    x -= IC_ROW_WIDTH;
    activateChip(false, true);
  } else {
    activateChip(true, false);
  }
  
  setICCursorPosition(row, x);
}

void LCDDisplay :: writeToMemory(uint8_t b) {
  switchReadMode(WRITE);
  digitalWrite(COMMAND_MEM_SWITCH_PIN, HIGH);
  busWrite(b);
  issueCommand();
}

bool LCDDisplay :: isDisplayOn() {
  updateState();
  return lastState.displayOn;
}

void LCDDisplay :: activateDisplay(bool activate) {
  switchReadMode(WRITE);
  digitalWrite(COMMAND_MEM_SWITCH_PIN, LOW);
  
  uint8_t CMD = 0x3E;
  if (activate) {
    CMD |= 1;
  }
  busWrite(CMD);
  issueCommand();
}

void LCDDisplay :: setVerticalScroll(int yoffset) {
  yoffset = yoffset % DISPLAY_HEIGHT;
  if (yoffset < 0) {
    yoffset += DISPLAY_HEIGHT;
  }
  
  digitalWrite(COMMAND_MEM_SWITCH_PIN, LOW);
  uint8_t DISPLAY_START_CMD = 0xC0;
  busWrite(DISPLAY_START_CMD | yoffset);
  issueCommand();
}

void LCDDisplay :: testPattern() {
  switchReadMode(WRITE);
  
  for (int page = 0; page < 8; page++) {
    digitalWrite(COMMAND_MEM_SWITCH_PIN, LOW);
    uint8_t PAGE_SELECT_CMD = 0xB8;
    busWrite(PAGE_SELECT_CMD | page);
    issueCommand();
    
    digitalWrite(COMMAND_MEM_SWITCH_PIN, LOW);
    uint8_t ADDRESS_SELECT_CMD = 0x40;
    busWrite(ADDRESS_SELECT_CMD | 0);
    issueCommand();
    
    digitalWrite(COMMAND_MEM_SWITCH_PIN, HIGH);
    for (int x = 0; x < 64; x++) {
      busWrite(x | ((page & 0x03) << 6));
      issueCommand();
    }
  }
  
  for (int i = 0; i < 32; i++) {
    digitalWrite(COMMAND_MEM_SWITCH_PIN, LOW);
    uint8_t DISPLAY_START_CMD = 0xC0;
    busWrite(DISPLAY_START_CMD | i);
    issueCommand();
    
    delay(100);
  }
  
  setVerticalScroll(0);
}

void LCDDisplay :: cls() {
  activateChip(true, true);
  for (int row = 0; row < ROW_COUNT; row++) {
    setICCursorPosition(row, 0);
    for (int x = 0; x < 64; x++) {
      writeToMemory(0);
    }
  }
}

void LCDDisplay :: writeRow(unsigned int row, unsigned int xoffset, unsigned int count, uint8_t* data) {
  if ((row >= ROW_COUNT) || (xoffset >= DISPLAY_WIDTH)) {
    return; // Nothing to do
  }
  
  if (xoffset + count > DISPLAY_WIDTH) {
    count = DISPLAY_WIDTH - xoffset;
  }
  
  setCursorPosition(row, xoffset);
  for (int x = xoffset; x < xoffset + count; x++) {
    if (x == IC_ROW_WIDTH) {
      // Must select second chip now
      setCursorPosition(row, x);
    }
    
    writeToMemory(*data);
    data++;
  }
}

void LCDDisplay :: writeImage(uint8_t* imgData) {
  for (int row = 0; row < ROW_COUNT; row++) {
    writeRow(row, 0, DISPLAY_WIDTH, imgData + row * DISPLAY_WIDTH);
  }
}

LCDDisplay :: LCDDisplay() {
  pinMode(RESET_PIN, OUTPUT);
  pinMode(CHIP1_SELECT_PIN, OUTPUT);
  pinMode(CHIP2_SELECT_PIN, OUTPUT);
  pinMode(COMMAND_MEM_SWITCH_PIN, OUTPUT);
  pinMode(WRITE_READ_SWITCH_PIN, OUTPUT);
  pinMode(ACTIVATE_COMMAND_PIN, OUTPUT);
  
  // Get into the default state we will use for the device:
  //   HIGH: ACTIVATE_COMMAND_PIN (ready to send command),
  //         CHIP1_SELECT_PIN,
  //         CHIP2_SELECT_PIN
  //         RESET_PIN (no reset)
  //  Other pins are undefined... will init to some initial state (state readmode)
  activateChip(true, true);
  digitalWrite(ACTIVATE_COMMAND_PIN, HIGH);
  digitalWrite(RESET_PIN, HIGH);
  
  // Set other pins to read-state settings
  updateState();

  // Issue a reset to get the device into a known state
  digitalWrite(RESET_PIN, LOW);
  digitalWrite(RESET_PIN, HIGH);
}


