Enttec Open DMX Plugin
======================

The plugin creates a single device with one output port using the Enttec
Open DMX USB widget. It requires the Open DMX kernel module, if you don't
have this installed, use the FTDI DMX USB plugin instead.


## Config file: `ola-opendmx.conf`

`device = /dev/dmx0`  
The path to the Open DMX USB device. Multiple entries are supported.
