E1.33 (RDMNet) Plugin
=====================================

This plugin creates a single device with a configurable number of input and
output ports.

Each port can be assigned to a different E1.33 Universe.


## Config file: `ola-e133.conf`

`cid = 00010203-0405-0607-0809-0A0B0C0D0E0F`  
The CID to use for this device.

#`dscp = [int]`  
#The DSCP value to tag the packets with, range is 0 to 63.

`input_ports = [int]`  
The number of input ports to create up to an arbitrary max of 512.

`ip = [a.b.c.d|<interface_name>]`  
The IP address or interface name to bind to. If not specified it will use
the first non-loopback interface.

`output_ports = [int]`  
The number of output ports to create up to an arbitrary max of 512.
