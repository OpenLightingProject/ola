Milford Instruments Plugin
==========================

This plugin creates devices with one output port. It currently supports the
1-463 DMX Protocol Converter and 1-553 512 Channel Serial to DMX Transmitter.


## Config file: `ola-milinst.conf`

`device = /dev/ttyS0`  
The device to use as a path for the serial port. Multiple devices are supported.

### Per Device Settings

`<device>-type = [1-463 | 1-553]`  
The type of interface.

#### 1-553 Specific Per Device Settings

`<device>-baudrate = [9600 | 19200]`  
The baudrate to connect at.

`<device>-channels = [128 | 256 | 512]`  
The number of channels to send.
