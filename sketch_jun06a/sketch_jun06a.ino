#include <stdint.h>

#include <SPI.h>
#include <SD.h>

const int DIGITAL_SOUND_PIN = 3;

const int SPI_PIN = 4; // Required for sd card connection!!!

class LCDDisplay {
  public:
    // Static properties of the display
    const static unsigned int ROW_COUNT = 8;
    const static unsigned int IC_ROW_WIDTH = 64;
    
    const static unsigned int DISPLAY_WIDTH = 2 * IC_ROW_WIDTH;
    const static unsigned int DISPLAY_HEIGHT = ROW_COUNT * 8;
  private:
    // Connection pins of the display
    const static int RESET_PIN = 22;
    
    const static int CHIP1_SELECT_PIN = 24;
    const static int CHIP2_SELECT_PIN = 25;
    
    const static int COMMAND_MEM_SWITCH_PIN = 26; // LOW = command, HIGH = memory
    const static int WRITE_READ_SWITCH_PIN = 27;  // LOW = write,   HIGH = read
    const static int ACTIVATE_COMMAND_PIN = 23;   // HIGH->LOW flank executes command, HIGH is the resting level
    
    const static int BUS_START_PIN = 28;
    const static int BUS_END_PIN = 35;

    struct State {
      bool displayOn;
      bool resetting;
      bool busy;
      
      State() : displayOn(false), resetting(false), busy(false) {}
    };
    
    enum ReadWriteMode {
      READ, WRITE
    };
    
    State lastState;
    
    void switchReadMode(ReadWriteMode mode) {
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
    
    void issueCommand() {
      // Pulses ACTIVATE_COMMAND_PIN, executing a command on the LCD
      digitalWrite(ACTIVATE_COMMAND_PIN, LOW);
      digitalWrite(ACTIVATE_COMMAND_PIN, HIGH);
    }
    
    void updateState() {
      switchReadMode(READ);
      digitalWrite(COMMAND_MEM_SWITCH_PIN, LOW);

      lastState.displayOn = digitalRead(BUS_START_PIN + 5) == LOW;
      lastState.resetting = digitalRead(BUS_START_PIN + 4) == HIGH;
      lastState.busy = digitalRead(BUS_START_PIN + 7) == HIGH;
    }
    
    // Clamps a byte of data to the bus - switches readmode to be sure
    void busWrite(uint8_t data) {
      switchReadMode(WRITE);
      for (int i = 0; i < 8; i++) {
        digitalWrite(BUS_START_PIN + i, (data >> i) & 1 != 0 ? HIGH : LOW);
      }
    }
    
    // Activates chips of the left or right half of the display (or both)
    void activateChip(bool left, bool right) {
      // When switching to another chip, make sure we read something in, 
      // otherwise we might issue involuntary commands on the chip
      // when it is being selected
      updateState();
      
      digitalWrite(CHIP1_SELECT_PIN, left  ? HIGH : LOW);
      digitalWrite(CHIP2_SELECT_PIN, right ? HIGH : LOW);
    }
    
    // sets the write position for the ICs
    void setICCursorPosition(unsigned int row, unsigned int x) {
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
    
    // same as setICCursorPosition, but selects the correct IC
    // based on the x (allowing x to fall in the range 0 <= x < DISPLAY_WIDTH)
    void setCursorPosition(unsigned int row, unsigned int x) {
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
    
    // writes data into memory at the current memory position
    void writeToMemory(uint8_t b) {
      switchReadMode(WRITE);
      digitalWrite(COMMAND_MEM_SWITCH_PIN, HIGH);
      busWrite(b);
      issueCommand();
    }
  public:
    bool isDisplayOn() {
      updateState();
      return lastState.displayOn;
    }
    
    void activateDisplay(bool activate) {
      switchReadMode(WRITE);
      digitalWrite(COMMAND_MEM_SWITCH_PIN, LOW);
      
      uint8_t CMD = 0x3E;
      if (activate) {
        CMD |= 1;
      }
      busWrite(CMD);
      issueCommand();
    }

    // Scrolls the screen vertically by n-pixels, moving the screen to the top.
    // Max value is DISPLAY_HEIGHT - 1. Values outside the range are "modded" (%)
    // into the interval [0, DISPLAY_HEIGHT - 1]. Pixel values that would fall outside
    // the diplsay range are rotated into view at the bottom of the screen.
    void setVerticalScroll(int yoffset) {
      yoffset = yoffset % DISPLAY_HEIGHT;
      if (yoffset < 0) {
        yoffset += DISPLAY_HEIGHT;
      }
      
      digitalWrite(COMMAND_MEM_SWITCH_PIN, LOW);
      uint8_t DISPLAY_START_CMD = 0xC0;
      busWrite(DISPLAY_START_CMD | yoffset);
      issueCommand();
    }

    // Small test pattern used during development to understand the paritioning of the 
    // display. Writes 8 lines with increasing integers, and scrolls them by modifying the
    // DISPLAY_START value - leading to the screen rolling upwards.    
    void testPattern() {
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
    
    // Functions to write to the display memory (efficiently and easy) //

    // Background:
    // The screen is divided into a left and right half, controlled by two ICs. 
    // Each IC contains memory contains memory which is segmented into 8 rows.
    // Each row contains 64 bytes, each byte describing a 8-pixel column in a 
    // row. The bits in a row are aligned vertically with the lowest-order bit
    // being at the top of a row, and the highest order bit at the bottom. This 
    // results in each IC controlling an area 64x64:
    //   64 (width) x 8 (rows) x 8 (bits) of the LCD display.
    
    // Since the display is segmented into 8 rows of 2x64 bytes (both ICs), I chose
    // to implement these functions in a way that allows one to write whole
    // or parital rows specifying the row number and X offset. Switching between 
    // the two ICs controlling each half of the screen will be done automatically.
    // This allows for efficient copying of memory to the display for showing, e.g.
    // images.
    
    // Clears the whole screen
    void cls() {
      activateChip(true, true);
      for (int row = 0; row < ROW_COUNT; row++) {
        setICCursorPosition(row, 0);
        for (int x = 0; x < 64; x++) {
          writeToMemory(0);
        }
      }
    }

    // writes a row of data into display memory (across IC boundaries)
    // Parameters:
    //   row     ->  0 <= row < ROW_COUNT; selects the row that should be written
    //   xoffset ->  x coordinate to begin writing from.
    //   count   -> number of bytes to write, automatically clamped to the width of the screen
    void writeRow(unsigned int row, unsigned int xoffset, unsigned int count, uint8_t* data) {
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
    
    // Very high level functions allowing to do stuff with the whole screen
    
    // Outputs an image to the LCD display
    //   Parameters:
    //     imgData -> must point to a ROW_COUNT * DISPLAY_WIDTH byte array containing all 
    //                data required for representing an image. All rows of the image
    //                are expected to follow one another in this repesentation.
    void writeImage(uint8_t* imgData) {
      for (int row = 0; row < ROW_COUNT; row++) {
        writeRow(row, 0, DISPLAY_WIDTH, imgData + row * DISPLAY_WIDTH);
      }
    }
    
    LCDDisplay() {
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
};

uint8_t MUSIC_PWM_DIVISORS[128] = {0xff, 127, 84, 63, 50, 42, 36, 31, 27, 25, 22, 20, 19, 17, 16, 15, 14, 13, 12, 12, 11, 11, 10, 10, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  
#include "TimerThree.h"
  
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
  pinMode(DIGITAL_SOUND_PIN, OUTPUT);
  
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
    Timer3.pwm(DIGITAL_SOUND_PIN, b * 4);
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
  digitalWrite(DIGITAL_SOUND_PIN, LOW);
  Serial.println("Done!");
  
  musicFile.close();
}

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
