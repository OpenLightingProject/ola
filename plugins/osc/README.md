OSC (Open Sound Control) Plugin
===============================

This plugin allows OLA to send and receive
[OSC](http://www.opensoundcontrol.org/) messages.

OLA uses the blob type for transporting DMX data.


## Config file: `ola-osc.conf`

`input_ports = <int>`  
The number of input ports to create.

`output_ports = <int>`  
The number of output ports to create.

`udp_listen_port = <int>`
The UDP Port to listen on for OSC messages.

`port_N_address = /address`  
The OSC address to listen on for port N. If the address contains `%d` it's
replaced by the universe number for port N.

`port_N_targets = ip:port/address,ip:port/address,...`
For output port N, the list of targets to send OSC messages to. If the
targets contain `%d` it's replaced by the universe number for port N

`port_N_output_format = <osc type>`
The format (OSC Type) to send the DMX data in:

- `blob`: a OSC-blob
- `float_array`: an array of float values. 0.0 - 1.0
- `individual_float`: one float message for each slot (channel). 0.0 - 1.0
- `individual_int`: one int message for each slot (channel). 0 - 255.
- `int_array`: an array of int values. 0 - 255.
