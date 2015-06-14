#include <stdint.h>

#include "LCD.h"

#include <SPI.h>
#include <SD.h>

#include <TimerOne.h>
  
const int DIGITAL_SOUND_PIN = 3;

const int SPI_PIN = 4; // Required for sd card connection!!!

LCDDisplay* lcd_display;

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
        Serial.println("Unexpected end of (image) file");
        break;
      }
      lcd_display->writeRow(row, 0, LCDDisplay::DISPLAY_WIDTH, rowData);
    }
  }
  file.close();
}

void setup() {
  // Debug output initialization
  Serial.begin(9600);

  // SD card initialization
  SD.begin(SPI_PIN);
  
  lcd_display = new LCDDisplay;
  
  lcd_display->cls();
  lcd_display->activateDisplay(true);
  
  uint8_t data[128];
  for (int i = 0; i < 128; i++) {
    data[i] = i;
  }
  
  tone(3, 440, 1000);

  lcd_display->cls();
  //lcd_display->writeRow(1, 0, 128, data);
  lcd_display->testPattern();

  displayImage("images/test.bim");

  //playMusic("music/VVVVV.rms");
}

void loop() {
}
