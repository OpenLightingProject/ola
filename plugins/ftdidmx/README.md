# FTDI USB Chipset DMX Plugin

This plugin is compatible with Enttec Open DMX USB and other FTDI chipset based
USB to DMX converters where the host needs to create the DMX stream itself
and not the interface (the interface has no microprocessor to do so).

## RDM Support

FTDI based chips/outputs that have the correct line biasing setup should be
able to output and receive RDM packets.

At this stage we can't guarantee that the plugin meets all timing specs since 
I don't have any faulty RDM equipment to test with. It has however been tested
with multiple responders on the line and discovery works correctly.

*It is also critical that the FTDI local echo function is disabled*

### Proper Line Biasing
For simple DMX output (and input) all that is needed is a 130 Ohm resistor 
between data+ and data-.

For RDM 2 additional resistors of 680 Ohm are needed:
1. Pull-up connects between Data+ and VCC
2. Pull-down between Data- and the common/ground.

### RDM was tested with
- FT4232H (USB-COM485-PLUS4)
- USB-RS485-WE-1800-BT

### RDM was tested as not working with
- Enttec Open DMX USB (we believe this may be due to incorrect line biasing)

## Config file: ola-ftdidmx.conf

`frequency = 30`  
The DMX stream frequency (30 to 44 Hz max are the usual).
