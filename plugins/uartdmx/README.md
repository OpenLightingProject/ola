Native UART DMX Plugin
======================

This plugin drives a supported POSIX UART (plus extensions) to produce a
direct DMX output stream. The host needs to create the DMX stream itself as
there is no external microcontroller.

This is tested with the on-board UART of the Raspberry Pi. See here for a
possible schematic:
http://eastertrail.blogspot.co.uk/2014/04/command-and-control-ii.html


## Config file: `ola-uartdmx.conf`

`enabled = true`
Enable this plugin (DISABLED by default).

`device = /dev/ttyAMA0`
The device to use for DMX output (optional). Multiple devices are supported
if the hardware exists. On later software it may also be /dev/serial0.
Using USB-serial adapters is not supported (try the
*ftdidmx* plugin instead).


### Per Device Settings (using above device name)

`<device>-break = 100`
The DMX break time in microseconds for this device (optional).

`<device>-malf = 100`
The Mark After Last Frame time in microseconds for this device (optional).
