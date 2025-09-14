KiNET Plugin
============

This plugin creates a single device with multiple output ports. Each port
represents a power supply. This plugin supports both the DMX Out (V1) and
the Port Out (V2) modes of the KiNET protocol.


## Config file: `ola-kinet.conf`

`power_supply = <ip>`  
The IP of the power supply to send to. You can communicate with more than
one power supply by adding multiple `power_supply =` lines.

### Per Power Supply Settings

`<ip>-mode = [dmxout|portout]`  
The mode of KiNET to send to the power supply. DMX Out is sometimes known as
V1 and Port Out as V2.

In general, it is highly recommended to use the Port Out mode if your power
supply supports it. Only very old devices do not support Port Out mode, so
it is recommended to try Port Out mode first. OLA defaults to DMX Out mode
for backwards compatibility reasons. Note that some newer devices only
support the Port Out protocol. On some devices it is also necessary to use
the Port Out protocol in order to address all of the connected fixtures if
the number of channels required exceeds one universe.

Note that for the DMX Out mode the universe option on the power supply will
be ignored, as this plugin only supports unicast DMX Out packets destined
for the wildcard universe on each device. Instead, the universe for each
device may be patched by assigning this output port to the intended universe
in OLA.

`<ip>-ports = <int>`  
The number of physical ports available on the power supply in Port Out mode.
Each physical port will create an OLA port that may be assigned to any
universe. This setting is ignored in DMX Out mode. The default and maximum
number of ports per power supply is 16.
