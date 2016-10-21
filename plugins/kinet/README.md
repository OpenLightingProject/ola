KiNET Plugin
============

This plugin creates a single device with multiple output ports. Each port
represents a power supply. This plugin uses the V1 DMX-Out version of the
KiNET protocol.


## Config file: `ola-kinet.conf`

`power_supply = <ip>`  
The IP of the power supply to send to. You can communicate with more than
one power supply by adding multiple `power_supply =` lines
