/*
 * File:   main.c
 * Author: F. Dahlberg
 * 
 * Name of Device: LED Chaser
 * Function: LED chaser using a PIC12F629 microcontroller and two 74HC595N
 *           8 bit serial in parallel out shift registers
 * Created on July 8, 2022
 */

#include "CONFIG.h"

// Pattern 1 (Kitt) -- 0x00 to 0x1F
__EEPROM_DATA(0b10000000,0b00000000,0b01000000,0b00000000,0b00100000,0b00000000,0b00010000,0b00000000);
__EEPROM_DATA(0b00001000,0b00000000,0b00000100,0b00000000,0b00000010,0b00000000,0b00000001,0b00000000);
__EEPROM_DATA(0b00000000,0b10000000,0b00000000,0b01000000,0b00000000,0b00100000,0b00000000,0b00010000);
__EEPROM_DATA(0b00000000,0b00001000,0b00000000,0b00000100,0b00000000,0b00000010,0b00000000,0b00000001);

// Pattern 2 (In N Out) -- 0x20 to 0x2F
__EEPROM_DATA(0b10000000,0b00000001,0b01000000,0b00000010,0b00100000,0b00000100,0b00010000,0b00001000);
__EEPROM_DATA(0b00001000,0b00010000,0b00000100,0b00100000,0b00000010,0b01000000,0b00000001,0b10000000);

// Pattern 3 (Four Chaser) -- 0x30 to 0x37
__EEPROM_DATA(0b10001000,0b10001000,0b01000100,0b01000100,0b00100010,0b00100010,0b00010001,0b00010001);

// Pattern 4 (Waterfall) -- 0x38 to 0x57
__EEPROM_DATA(0b10000000,0b00000001,0b11000000,0b00000011,0b11100000,0b00000111,0b11110000,0b00001111);
__EEPROM_DATA(0b11111000,0b00011111,0b11111100,0b00111111,0b11111110,0b01111111,0b11111111,0b11111111);
__EEPROM_DATA(0b01111111,0b11111110,0b00111111,0b11111100,0b00011111,0b11111000,0b00001111,0b11110000);
__EEPROM_DATA(0b00000111,0b11100000,0b00000011,0b11000000,0b00000001,0b10000000,0b00000000,0b00000000);

// Pattern 5 (Chaser) -- 0x58 to 0x78
__EEPROM_DATA(0b00000000,0b00110000,0b00000000,0b00010100,0b00000000,0b00001001,0b01000000,0b00000100);
__EEPROM_DATA(0b00010000,0b00000010,0b00000100,0b00000001,0b10000001,0b00000000,0b01000000,0b01000000);
__EEPROM_DATA(0b00100000,0b00010000,0b00010000,0b00000100,0b00001000,0b00000001,0b01000100,0b00000000);
__EEPROM_DATA(0b00010010,0b00000000,0b00000101,0b00000000,0b00000001,0b10000000,0b00000000,0b01000000);

#define _XTAL_FREQ 4000000

#define DATAOUT GPIO0
#define LATCH GPIO1
#define SHIFTCLOCK GPIO2
#define DELAYBUTTON GPIO4
#define PATTERNBUTTON GPIO5
#define PATTERN_DELAY_MILLIS 5
#define NUMBER_OF_PATTERNS 5

// Function pre definitions
void shiftOutByte(char x);
void shiftRegisterClockPulse();
void shiftRegisterLatchPulse();
void checkDelayButton();
void checkPatternButton();

// Struct array holding the definitions of the patterns in EEPROM
struct patternDef patternCatalog[5];
int catalogChoice = 0;  // Which pattern is currently selected to display

// Apparently there's a requirement that delay time values are compile time
// static and can not be configured. Therefore, we delay a constant time, but
// just run a for loop through iterations of the fixed delay value.
int delayCount = 3;

// Global variable used to hold store a byte from EEPROM in memory before
// shifting it out to the shift registers.
char byteToWrite;

// Flag indicating whether the delay set pushbutton has been pressed.
// Used so that we only react to a press on the positive trigger edge.
bool delayButtonPressedFlag = false;
bool patternButtonPressedFlag = false;
bool changePatternNow = false;
bool reversing = false;
int delayButtonState;
int patternButtonState;

/**
 * Initializes the microcontroller registers and all global variables
 * to expected starting conditions.
 */
void initialize() {
    TRISIO = 0b00110000; // GPIO 4 and 5 are inputs, all others are outputs
    CMCON = 7;
    GPIO = 0; // Initialize all pins to 0
    
    patternCatalog[0].size = 32;
    patternCatalog[0].startLocation = 0;
    patternCatalog[1].size = 16;
    patternCatalog[1].startLocation = 32;
    patternCatalog[2].size = 8;
    patternCatalog[2].startLocation = 48;;
    patternCatalog[3].size = 32;
    patternCatalog[3].startLocation = 56;
    patternCatalog[4].size = 32;
    patternCatalog[4].startLocation = 88;

    return;
}

/**
 * Cycles the shift register clock to clock in a bit.
 */
void shiftRegisterClockPulse() {
    SHIFTCLOCK = 1;
    SHIFTCLOCK = 0;
    
    return;
}

/**
 * Cycles the shift register latch to latch out all register values.
 */
void shiftRegisterLatchPulse() {
    LATCH = 1;
    LATCH = 0;
    
    return;
}

/** 
 * Shift a byte out of the data pin onto a shift register, cycling the clock
 * after each bit is shifted out.
 */
void shiftOutByte(char byteToSend) {  
    for(int i=0; i<8; i++) {
        DATAOUT = (byteToSend >> i) & (0x01);
        shiftRegisterClockPulse();
    }
    
    return;
}

/**
 * Checks if the delay button has been pressed.  If the delay button has
 * been pressed and the delay button flag is false (indicating a new press
 * of the button), then we set the delay button flag to true so that the
 * main loop can act upon the new input.
 */
void checkDelayButton() {
    if(DELAYBUTTON == 1 && !delayButtonPressedFlag) {
        delayButtonPressedFlag = true;
        delayCount += 6;                         
    
        if(delayCount >= 39) {
            delayCount =3;
        }
    }else if(DELAYBUTTON != 1) {
        delayButtonPressedFlag = false;
    }
    
    return;
}

/**
 * Checks if the pattern button has been pressed.  If the pattern button has
 * been pressed and the pattern button flag is false (indicating a new press
 * of the button), then we set the pattern button flag to true so that the
 * main loop can act upon the new input.
 */
void checkPatternButton() {
    if(PATTERNBUTTON == 1 && !patternButtonPressedFlag) {
        patternButtonPressedFlag = true;
        // We use two booleans to register a pattern change instead of one, because the 
        // first boolean flags this function not to register the button press more then once,
        // and the second boolean flags the main loop not to change the pattern more then once.
        // If we tried to do both actions in one boolean, then we'd risk that the main loop
        // could see the flag to change patterns, change patterns, set the flag down, and start
        // a new loop while the user was still pressing the button which would register another
        // pattern change.  patternButtonPressedFlag is the flag for this function, and 
        // changePatternNow is the flag for the main loop to actually change patterns.
        changePatternNow = true;
        
        // Before switching patterns, we enable reversing the current pattern.  If the current 
        // pattern is already reverse enabled, switch back to normal and disable reverse.
        if(!reversing) {
            reversing = true;
        } else {
            reversing = false;
            catalogChoice++;
        }
        
        if(catalogChoice >= NUMBER_OF_PATTERNS) {
            catalogChoice = 0;
        }
    } else if(PATTERNBUTTON != 1) {
        patternButtonPressedFlag = false;
    }
    
    return;
}

void main(void) {
    initialize();
    
    // Main program loop
    while(1) {
        for(int i = patternCatalog[catalogChoice].startLocation; 
                i < (patternCatalog[catalogChoice].startLocation + patternCatalog[catalogChoice].size) 
                && !changePatternNow;) {
            byteToWrite = eeprom_read(i);
            shiftOutByte(byteToWrite);
            i++;
            byteToWrite = eeprom_read(i);
            shiftOutByte(byteToWrite);
            i++;
            shiftRegisterLatchPulse();
            // Checking the delay button before and after the loop since the
            // rest of the loop executes quickly.
            checkDelayButton();
            checkPatternButton();
            // Loop through a number of delays (it's a requirement that the actual
            // delay times are compile time final, so we loop through a variable 
            // number of them).
            for(int j=0; j<delayCount; j++) {
                __delay_ms(PATTERN_DELAY_MILLIS);
            }
            checkDelayButton();
            checkPatternButton();
        }
        
        // If we don't reverse this pattern, continue past the next for loop        
        if (!reversing) {
            changePatternNow = false;
            continue;
        }
        
        // We start the loop two elements below the end of the pattern, since the last two bytes have
        // just been displayed.  We end the loop two elements above the start of the pattern, since the
        // first thing the next iteration of the loop will show is the first two pattern elements.
        for(int i = (patternCatalog[catalogChoice].startLocation + patternCatalog[catalogChoice].size - 3); 
                i >= (patternCatalog[catalogChoice].startLocation + 2)
                && !changePatternNow;) {
            if(changePatternNow) {
                break;
            }
            byteToWrite = eeprom_read(i-1);
            shiftOutByte(byteToWrite);
            byteToWrite = eeprom_read(i);
            shiftOutByte(byteToWrite);
            i-=2;
            shiftRegisterLatchPulse();
            // Loop through a number of delays (it's a requirement that the actual
            // delay times are compile time final, so we loop through a variable 
            // number of them).
            checkDelayButton();
            checkPatternButton();
            for(int j=0; j<delayCount; j++) {
                __delay_ms(PATTERN_DELAY_MILLIS);
            }
            checkDelayButton();
            checkPatternButton();
        }
        
        changePatternNow = false;
    }
    return;
}
