SandNet Plugin
==============

This plugin creates a single device with 2 output and 8 input ports.

The universe bindings are offset by one from those displayed in sandnet. For
example, SandNet universe 1 is OLA universe 0.


## Config file: `ola-sandnet.conf`

`ip = [a.b.c.d|<interface_name>]`  
The IP address or interface name to bind to. If not specified it will use
the first non-loopback interface.

`name = ola-SandNet`  
The name of the node.
