Strand ShowNet Plugin
=====================

This plugin creates a single device with 8 input and 8 output ports.

The ports correspond to the DMX channels used in the shownet protocol. For
example the first input and output port 0 is channels 1 - 512 and the second
input and output ports are channels 513 - 1024.


## Config file: `ola-shownet.conf`

`ip = [a.b.c.d|<interface_name>]`  
The IP address or interface name to bind to. If not specified it will use
the first non-loopback interface.

`name = ola-ShowNet`  
The name of the node.
