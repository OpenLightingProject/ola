OPC Plugin
==========

This plugin creates OPC Client and/or OPC Server Devices.


## Config file: `ola-opc.conf`

`target = <IP>:<port>`  
Create a Open Pixel Control client, connected to the `IP:port`. Multiple
targets can be specified and a device will be created for each.

`listen = <IP>:<port>`  
Create an Open Pixel Control server, listening on `IP:port`. To listen on
any address use `listen = 0.0.0.0`. Multiple listen keys can be specified
and a device will be created for each.

`target_<IP>:<port>_channel = <channel>`  
The Open Pixel Control channels to use for the specified device. Multiple
channels can be specified and an output port will be created for each.

`listen_<IP>:<port>_channel = <channel>`  
The Open Pixel Control channels to use for the specified device. Multiple
channels can be specified and an input port will be created for each.
