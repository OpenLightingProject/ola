FTDI USB Chipset DMX Plugin
===========================

This plugin is compatible with Enttec Open DMX USB and other FTDI chipset 
based USB to DMX converters where the host needs to create the DMX stream 
itself and not the interface (the interface has no microprocessor to do so).

## RDM Support

FTDI based chips/outputs that have the correct line biasing setup should be
able to output and receive RDM packets.

At this stage we can't guarantee that the plugin meets all timing specs since 
I don't have any faulty RDM equipment to test with. It has however been tested
with multiple responders on the line and discovery works correctly.

Ideally FTDI local echo should be disabled to prevent transimitted data from 
being read.
The driver does have echo detection and attempts to correct for it, however
we can't commit to this working 100% off the time and it will add 
unpredictability.

The same applies for line biasing, things do seem to work without it but we
don't recommend it.

### Proper Line Biasing
For simple DMX output (and input) all that is needed is a 130 Ohm resistor
between data+ and data-.

For RDM 2 additional resistors of 680 Ohm are needed:
1.  Pull-up connects between Data+ and VCC
2.  Pull-down between Data- and the common/ground.

*Please note:* these values are based on the book "Control Freak" by Simon
Howell and are what I used in my test setup, however the standard actually
proscribes 133 Ohm and 562 Ohm resistors.

#### Diagram
     +V
     ---
      |
      +----------+
      |          |
      |       [680 Ohm]
    |\|          |
    | \----------+---------- DMX Pin 3 (Data+)
    |  \         |
    |   \     [130 Ohm]
    |   /        |
    |  /         |
    | /o---------+---------- DMX Pin 2 (Data-)
    |/|          |
      |       [680 Ohm]
      |          |
      +----------+---------- DMX Pin 1 (Common)
      |
      +-------[<=20 Ohm]---+
      |                    |
    Common               -----
                          ---
                           -

#### FTDI Board DB9 pinouts
Based on the FTDI spec this is the pinout to be used on their DB9 connectors
and the way to connect the lines and resistors.

1.  Data- (130 Ohm -> 3, 680 Ohm -> 5)
2.  Data+ (680 Ohm -> 9)
3.  130 Ohm -> 1
4.  Not used
5.  GND (680 Ohm -> 1)
6.  Not used
7.  Short with 8 to disable echo
8.  Short with 7 to disable echo
9.  +5VDC (680 Ohm -> 2)

The FTDI spec and the DMX/RDM spec disagree where the 130 Ohm resistor should
terminate, it could be that really it should be between pins 1 and 2, both 
ways have worked for me for pure output, for RDM I only tested this way.

### RDM was tested with
-   FT4232H (USB-COM485-PLUS4)
-   USB-RS485-WE-1800-BT

### RDM was tested as not working with
-   Enttec Open DMX USB (we believe this may be due to incorrect line biasing)

## Config file: ola-ftdidmx.conf

`frequency = 30`  
The DMX stream frequency (30 to 44 Hz max are the usual).
