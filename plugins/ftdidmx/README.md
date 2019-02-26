FTDI USB Chipset DMX Plugin
===========================

This plugin is compatible with Enttec OpenDmx and other FTDI chipset based
USB to DMX converters where the host needs to create the DMX stream itself
and not the interface (the interface has no microprocessor to do so).

RDM Support
===========

FTDI based chips/outputs that have the correct line biasing setup should be
able to output and receive RDM packets.

At this stage not all timings are correct.

RDM was tested with:
- FT4232H (USB-COM485-PLUS4)

## Config file: ola-ftdidmx.conf

`frequency = 30`  
The DMX stream frequency (30 to 44 Hz max are the usual).
