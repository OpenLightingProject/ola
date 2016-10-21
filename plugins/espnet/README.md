Enttec ESP Net Plugin
=====================

This plugin creates a single device with five input and five output ports.

ESP supports up to 255 universes. As ESP has no notion of ports, we provide a
fixed number of ports which can be patched to any universe. When sending data
from a port, the data is addressed to the universe the port is patched to. For
example if port 0 is patched to universe 10, the data will be sent to ESP
universe 10.


## Config file: `ola-espnet.conf`

`ip = [a.b.c.d|<interface_name>]`  
The ip address or interface name to bind to. If not specified it will use the
first non-loopback interface.

`name = ola-EspNet`  
The name of the node.
