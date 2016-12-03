ArtNet Plugin
=============

This plugin creates a single device with four input and four output ports
and supports ArtNet, ArtNet 2 and ArtNet 3.

ArtNet limits a single device (identified by a unique IP) to four input and
four output ports, each bound to a separate ArtNet Port Address (see the
ArtNet spec for more details). The ArtNet Port Address is a 16 bits int,
defined as follows:

| Bit 15 | Bits 14 - 8 | Bits 7 - 4 | Bits 3 - 0 |
| ------ | ----------- | ---------- | ---------- |
| 0      | Net         | Sub-Net    | Universe   |

For OLA, the Net and Sub-Net values can be controlled by the config file.
The Universe bits are the `OLA Universe number modulo 16`.

| ArtNet Net | ArtNet Subnet | OLA Universe | ArtNet Port Address |
| ---------- | ------------- | ------------ | ------------------- |
| 0          | 0             | 0            | 0                   |
| 0          | 0             | 1            | 1                   |
| 0          | 0             | 15           | 15                  |
| 0          | 0             | 16           | 0                   |
| 0          | 0             | 17           | 1                   |
| 0          | 1             | 0            | 16                  |
| 0          | 1             | 1            | 17                  |
| 0          | 15            | 0            | 240                 |
| 0          | 15            | 15           | 255                 |
| 1          | 0             | 0            | 256                 |
| 1          | 0             | 1            | 257                 |
| 1          | 0             | 15           | 271                 |
| 1          | 1             | 0            | 272                 |
| 1          | 15            | 0            | 496                 |
| 1          | 15            | 15           | 511                 |

That is `Port Address = (Net << 8) + (Subnet << 4) + (Universe % 16)`


## Config file: `ola-artnet.conf`

`always_broadcast = [true|false]`  
Use ArtNet v1 and always broadcast the DMX data. Turn this on if you have
devices that don't respond to ArtPoll messages.

`ip = [a.b.c.d|<interface_name>]`  
The ip address or interface name to bind to. If not specified it will use
the first non-loopback interface.

`long_name = ola - ArtNet node`  
The long name of the node.

`net = 0`  
The ArtNet Net to use (0-127).

`output_ports = 4`  
The number of output ports (Send ArtNet) to create. Only the first 4 will
appear in ArtPoll messages

`short_name = ola - ArtNet node`  
The short name of the node (first 17 chars will be used).

`subnet = 0`  
The ArtNet subnet to use (0-15).

`use_limited_broadcast = [true|false]`  
When broadcasting, use the limited broadcast address `255.255.255.255`
rather than the subnet directed broadcast address. Some devices which don't
follow the ArtNet spec require this.

`use_loopback = [true|false]`  
Enable use of the loopback device.
