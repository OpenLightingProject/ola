Pathway Pathport Plugin
=======================

This plugin creates a single device with 5 input and 5 output ports.

The universe the port is patched to corresponds with the DMX channels used in
the PathPort protocol. For example universe 0 is xDMX channels 0 - 511, universe
1 is xDMX channels 512 - 1023.


## Config file: `ola-pathport.conf`

`dscp = <int>`  
Set the DSCP value for the packets. Range is 0-63.

`ip = [a.b.c.d|<interface_name>]`  
The ip address or interface name to bind to. If not specified it will use the
first non-loopback interface.

`name = ola-Pathport`  
The name of the node.

`node-id = <int>`  
The pathport id of the node.
