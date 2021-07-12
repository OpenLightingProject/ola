E1.31 (Streaming DMX over ACN) Plugin
=====================================

This plugin creates a single device with a configurable number of input and
output ports.

Each port can be assigned to a different E1.31 Universe.


## Config file: `ola-e131.conf`

`cid = 00010203-0405-0607-0809-0A0B0C0D0E0F`  
The CID to use for this device.

`dscp = [int]`  
The DSCP value to tag the packets with, range is 0 to 63.

`draft_discovery = [bool]`  
Enable the draft (2014) E1.31 discovery protocol.

`ignore_preview = [true|false]`  
Ignore preview data.

`input_ports = [int]`  
The number of input ports to create up to an arbitrary max of 512.

`ip = [a.b.c.d|<interface_name>]`  
The IP address or interface name to bind to. If not specified it will use
the first non-loopback interface.

`output_ports = [int]`  
The number of output ports to create up to an arbitrary max of 512.

`prepend_hostname = [true|false]`  
Prepend the hostname to the source name when sending packets.

`revision = [0.2|0.46]`  
Select which revision of the standard to use when sending data. 0.2 is the
standardized revision, 0.46 (default) is the ANSI standard version.
