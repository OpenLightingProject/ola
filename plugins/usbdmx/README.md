USB DMX Plugin
==============

This plugin supports various USB DMX devices including:

* Anyma uDMX
* AVLdiy D512
* DMXControl Projects e.V. Nodle U1
* DMXCreator 512 Basic
* Eurolite USB-DMX512 PRO
* Eurolite USB-DMX512 PRO MK2 (when `enable_eurolite_mk2 = true`)
* Fadecandy
* ShowJockey SJ-DMX-U1
* Sunlite USBDMX2
* Velleman K8062.


## Config file : `ola-usbdmx.conf`

`libusb_debug_level = {0,1,2,3,4}`  
The debug level for libusb, see http://libusb.sourceforge.net/api-1.0/  
0 = No logging, 4 = Verbose debug.

`enable_eurolite_mk2 = {false,true}`
Whether to enable detection of the Eurolite USB-DMX512 PRO MK2.
Default = false. This device is indistinguishable from other devices
with an FTDI chip, and is therefore disabled by default. When enabled,
this plugin will conflict with the usbserial, StageProfi and FTDI USB DMX
plugins.

`nodle-<serial>-mode = {0,1,2,3,4,5,6,7}`  
The mode for the Nodle U1 interface with serial number `<serial>` to operate
in. Default = 6  
0 - Standby  
1 - DMX In -> DMX Out  
2 - PC Out -> DMX Out  
3 - DMX In + PC Out -> DMX Out  
4 - DMX In -> PC In  
5 - DMX In -> DMX Out & DMX In -> PC In  
6 - PC Out -> DMX Out & DMX In -> PC In  
7 - DMX In + PC Out -> DMX Out & DMX In -> PC In
