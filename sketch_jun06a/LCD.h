#ifndef LCD_H_
#define LCD_H_

#include <Arduino.h>

#include <stdint.h>

class LCDDisplay {
  public:
    // Static properties of the display
    const PROGMEM static unsigned int ROW_COUNT = 8;
    const PROGMEM static unsigned int IC_ROW_WIDTH = 64;
    
    const PROGMEM static unsigned int DISPLAY_WIDTH = 2 * IC_ROW_WIDTH;
    const PROGMEM static unsigned int DISPLAY_HEIGHT = ROW_COUNT * 8;
  private:
    // Connection pins of the display
    const PROGMEM static int RESET_PIN = 22;
    
    const PROGMEM static int CHIP1_SELECT_PIN = 24;
    const PROGMEM static int CHIP2_SELECT_PIN = 25;
    
    const PROGMEM static int COMMAND_MEM_SWITCH_PIN = 26; // LOW = command, HIGH = memory
    const PROGMEM static int WRITE_READ_SWITCH_PIN = 27;  // LOW = write,   HIGH = read
    const PROGMEM static int ACTIVATE_COMMAND_PIN = 23;   // HIGH->LOW flank executes command, HIGH is the resting level
    
    const PROGMEM static int BUS_START_PIN = 28;
    const PROGMEM static int BUS_END_PIN = 35;

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
    
    void switchReadMode(ReadWriteMode mode);

    void issueCommand();
    
    void updateState();
    
    // Clamps a byte of data to the bus - switches readmode to be sure
    void busWrite(uint8_t data);
    
    // Activates chips of the left or right half of the display (or both)
    void activateChip(bool left, bool right);
    
    // sets the write position for the ICs
    void setICCursorPosition(unsigned int row, unsigned int x);
    
    // same as setICCursorPosition, but selects the correct IC
    // based on the x (allowing x to fall in the range 0 <= x < DISPLAY_WIDTH)
    void setCursorPosition(unsigned int row, unsigned int x);
    
    // writes data into memory at the current memory position
    void writeToMemory(uint8_t b);
  public:
    bool isDisplayOn();
    
    void activateDisplay(bool activate);

    // Scrolls the screen vertically by n-pixels, moving the screen to the top.
    // Max value is DISPLAY_HEIGHT - 1. Values outside the range are "modded" (%)
    // into the interval [0, DISPLAY_HEIGHT - 1]. Pixel values that would fall outside
    // the diplsay range are rotated into view at the bottom of the screen.
    void setVerticalScroll(int yoffset);

    // Small test pattern used during development to understand the paritioning of the 
    // display. Writes 8 lines with increasing integers, and scrolls them by modifying the
    // DISPLAY_START value - leading to the screen rolling upwards.    
    void testPattern();
    
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
    void cls();

    // writes a row of data into display memory (across IC boundaries)
    // Parameters:
    //   row     ->  0 <= row < ROW_COUNT; selects the row that should be written
    //   xoffset ->  x coordinate to begin writing from.
    //   count   -> number of bytes to write, automatically clamped to the width of the screen
    void writeRow(unsigned int row, unsigned int xoffset, unsigned int count, uint8_t* data);
    
    // fills a display row (or part of it) with a constant value
    // Parameters:
    //   row     ->  0 <= row < ROW_COUNT; selects the row that should be written
    //   xoffset ->  x coordinate to begin writing from.
    //   count   -> number of bytes to write, automatically clamped to the width of the screen
    void fillRow(unsigned int row, unsigned int xoffset, unsigned int count, uint8_t value);
    
    // Very high level functions allowing to do stuff with the whole screen
    
    // Outputs an image to the LCD display
    //   Parameters:
    //     imgData -> must point to a ROW_COUNT * DISPLAY_WIDTH byte array containing all 
    //                data required for representing an image. All rows of the image
    //                are expected to follow one another in this repesentation.
    void writeImage(uint8_t* imgData);
    
    LCDDisplay();
};


#endif // LCD_H_

