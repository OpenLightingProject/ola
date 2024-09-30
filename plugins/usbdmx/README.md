USB DMX Plugin
==============

This plugin supports various USB DMX devices including:

* Anyma uDMX
* AVLdiy D512
* Digital Enlightenment USB-DMX
* DMXControl Projects e.V. Nodle U1
* DMXCreator 512 Basic
* Eurolite USB-DMX512 PRO
* Eurolite USB-DMX512 PRO MK2 (see notes below)
* Eurolite freeDMX Wi-Fi
* Fadecandy
* FX5 DMX
* ShowJockey SJ-DMX-U1
* Sunlite USBDMX2
* Nicoleaudie Sunlite intelligent USB DMX interface (SIUDI) (also ADJ MyDMX)
* Velleman K8062.


## Config file : `ola-usbdmx.conf`

`libusb_debug_level = {0,1,2,3,4}`
The debug level for libusb, see http://libusb.sourceforge.net/api-1.0/
0 = No logging, 4 = Verbose debug.

`enable_eurolite_mk2 = {false,true}`
Whether to enable detection of the Eurolite USB-DMX512 PRO MK2.
Default = `false`. This device is indistinguishable from other devices
with an FTDI chip, and is therefore disabled by default. When enabled,
this plugin will conflict with the usbserial, StageProfi and FTDI USB DMX
plugins. If this is undesirable, the `eurolite_mk2_serial` setting can be
used instead, which manually marks a specific USB device as a Eurolite
USB-DMX512 PRO MK2.

`eurolite_mk2_serial = <serial>`
Claim the USB device with the given serial number as a Eurolite USB-DMX512
PRO MK2 even when `enable_eurolite_mk2 = false`. This makes it possible
to use the Eurolite USB-DMX512 PRO MK2 together with other devices that
can not be distinguished otherwise. This setting has no effect when
`enable_eurolite_mk2 = true` or if no device is connected with the given
serial. The setting may be specified multiple times to use multiple Eurolite
USB-DMX512 PRO MK2 devices.

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
