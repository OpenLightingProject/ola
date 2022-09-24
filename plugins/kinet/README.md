KiNET Plugin
============

This plugin creates a single device with multiple output ports. Each port
represents a power supply. This plugin supports both the V1 DMX out and the
V2 Port out modes of the KiNET protocol.


## Config file: `ola-kinet.conf`

`power_supply = <ip>`
The IP of the power supply to send to. You can communicate with more than
one power supply by adding multiple `power_supply =` lines.

`<ip>-mode = [v1dmxout|v2portout]`
The mode of KiNET to send to the power supply.

In general, it is highly recommended to use the V2 Port out mode if your power
supply supports it. Note that some newer devices only support the V2 Port out
protocol. On some devices it is also necessary to use the V2 Port out protocol
in order to address all of the connected fixtures if the number of channels
required exceeds one universe.

Note that for DMX out mode, the universe option on the power supply will be
ignored, as this plugin supports only unicast DMX out packets. Instead, the
universe may be patched by assigning this output port to the intended universe
in OLA.

`<ip>-ports = [int]`
The number of physical ports available on the power supply in V2 Port out mode.
Each physical port will create an OLA port that may be assigned to any
universe. This setting is ignored in V1 DMX out mode.
