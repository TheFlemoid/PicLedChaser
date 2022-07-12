# PicLedChaser

## LED Chaser Based on the PIC12F629 Microcontroller

This is a quick project I put together in order to mess around with programming microcontrollers.
It utilizes the PIC12F629 since they're cheap, I happened to have a bunch of them, and they're "resource
scarce" enough to make things interesting.  All of the LED patterns that are displayed are stored in
EEPROM, and an array is stored on the heap that maintains knowledge of how many patterns there are,
what their starting registers in EEPROM are, and how big they are.  The main loop basically boils down
to:
 - Figure out what two bytes I should read from EEPROM
 - Read those bytes and shift them out to the shift registers
 - Iterate my local variables so that I can read the next two bytes from memory
 - Wait until I need to load the next two bytes

If you want to compile the application from source, the source files are in the /src directory.  If you
just want to flash the application onto an MCU, the /dist directory contains the necessary hex file.
The pdf file in the root directory contains the schematic diagram of the resultant circuit.  It should
be powered with 5 volts.
