Serial USB Plugin
=================

This plugin supports DMX USB devices that emulate a serial port. This
includes:

* Arduino RGB Mixer
* DMX-TRI & RDM-TRI
* DMXking USB DMX512-A, Ultra DMX, Ultra DMX Pro
* DMXter4, DMXter4A & mini DMXter
* Enttec DMX USB Pro & USB Pro Mk II
* Robe Universe Interface
* LumenRadio CRMX Nova TX USB
* Shantea Controls OpenDeck

See https://wiki.openlighting.org/index.php/USB_Protocol_Extensions for 
more info.


## Config file: `ola-usbserial.conf`

`device_dir = /dev`  
The directory to look for devices in.

`device_prefix = ttyUSB`  
The prefix of filenames to consider as devices. Multiple keys are allowed.

`ignore_device = /dev/ttyUSB`  
Ignore the device matching this string. Multiple keys are allowed.

`opendeck_fps_limit = 40`  
The max frames per second to send to a OpenDeck device.

`pro_fps_limit = 190`  
The max frames per second to send to a Usb Pro or DMXKing device.

`tri_use_raw_rdm = [true|false]`  
Bypass RDM handling in the {DMX,RDM}-TRI widgets.

`ultra_fps_limit = 40`  
The max frames per second to send to a Ultra DMX Pro device.
