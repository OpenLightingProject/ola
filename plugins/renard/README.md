Renard Plugin
=============

This plugin creates devices with one output port. It supports multiple
Renard SS boards directly connected, or daisy-chained. See this link for
more information:
http://www.doityourselfchristmas.com/wiki/index.php?title=Renard


## Config file: `ola-renard.conf`

`device = /dev/ttyUSB0`  
The device to use as a path for the serial port or USB-Serial adapter.
Renard boards don't have a built-in USB port, you will need an adapter
(USB->RS232 or USB->RS485). Multiple devices are supported.

### Per Device Settings (using above device name without `/dev/`)
`<device>-baudrate = <int>`  
The speed of the serial port, options are 9600, 19200, 38400, 57600, 115200.
Default is 57600.

`<device>-channels = <int>`  
The number of channels connected to this device. Default 64. Note that the
max number of channels vary by baud rate and the encoding.

`<device>-dmx-offset = <int>`  
Which starting point in the DMX universe this device is mapped to. The
default is 0, which means the first channel on Renard address 128 (0x80)
will be mapped to DMX channel 1.
