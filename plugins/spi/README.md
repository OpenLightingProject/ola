SPI Plugin
==========

This plugin enables control of LED pixel strings using SPI. Each SPI output
is represented as an OLA Device. Devices can have multiple Ports, each of
which controls a pixel string. Each Port can use a different personality
(pixel type) and DMX start address, this allows a combination of various
strings lengths & pixel hardware types. The start address and personality
settings are controllable via RDM (each Port appears as a RDM responder).

To support multiple ports per SPI output, we use an SPI-Backend. Two
backends are supported right now, a software backend which concatenates all
the pixel data into a single buffer and a hardware multiplexer backend which
uses the GPIO pins to control an off-host multiplexer. It's recommended to
use the hardware multiplexer.


## Config file: `ola-spi.conf`

`base_uid = <string>`  
The starting UID to use for the SPI RDM, e.g. `7a70:00000100`.

`device_prefix = <string>`  
The prefix of files to match in `/dev`. Usually set to `spidev`. Each match
will instantiate a Device.

### Per Device Settings

`<device>-spi-speed = <int>`  
The speed of the SPI bus, range is 0 - 32000000 Hz.

`<device>-ce-high = <bool>`  
The mode of the CE pin. Set to false this pulls the CE pin low when writing
data. Set to true this will pull the pin high when writing.

`<device>-backend = [software | hardware]`  
The backend to use to multiplex the SPI data.

`<device>-gpio-pin = <int>`  
The GPIO pins to use for the hardware multiplexer. Add one line for each
pin. The number of ports will be 2 ^ (# of pins).

`<device>-ports = <int>`  
If the software backend is used, this defines the number of ports which will
be created.

`<device>-sync-ports = <int>`  
Controls which port triggers a flush (write) of the SPI data. If set to -1
the SPI data is written when any port changes. This can result in a lot of
data writes (slow) and partial frames. If set to -2, the last port is used.


### Per Port Settings

Ports are indexed from 0.

`<device>-<port>-dmx-address = <int>`  
The DMX address to use. e.g. `spidev0.1-0-dmx-address = 1`

`<device>-<port>-device-label = <string>`  
The RDM device label to use.

`<device>-<port>-personality = <int>`  
The RDM personality to use.

`<device>-<port>-pixel-count = <int>`  
The number of pixels for this port. e.g. `spidev0.1-1-pixel-count = 20`
