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
The number of input ports to create up to a max of 32.

`ip = [a.b.c.d|<interface_name>]`  
The IP address or interface name to bind to. If not specified or no
matching interface is found the first "up" or configured non-loopback
interface is used. Prefer `interface` if this is not desired.

`interface = <interface_name>`
The name of the interface to bind to. Overrides the "ip" option if given.
Unlike the "ip" option this will use the specified interface whenever
possible at all, especially even if it is down. If the interface does not
exist an error is thrown rather than picking just any interface.

`output_ports = [int]`  
The number of output ports to create up to a max of 32.

`prepend_hostname = [true|false]`  
Prepend the hostname to the source name when sending packets.

`revision = [0.2|0.46]`  
Select which revision of the standard to use when sending data. 0.2 is the
standardized revision, 0.46 (default) is the ANSI standard version.
