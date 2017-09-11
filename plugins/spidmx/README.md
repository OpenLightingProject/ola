Native SPI DMX Plugin
=====================

This plugin samples the Serial Peripheral Interface (SPI) at 2MHz to
receive DMX data. You need to connect only the *MISO* pin to the *receive*
pin of a transceiver chip like the *MAX485* or *SN75176*.

Repeatedly, blocks of 4096 bytes are read as a batch operation and parsed
afterwards. We must hope that a DMX packet is contained in a block as a
whole. Since often a packet will be cut off by the block end, the higher
channels are updated less frequently than the lower ones. However, you can
set a larger block size if you want a higher refresh rate for the high
channels (which results in more stuttering in all channels, as the time
between two SPI read operation increases).


## Raspberry Pi configuration

On the Raspberry Pi, you need to enable SPI and increase `spidev`'s
read / transmit buffer size:

* Run `sudo raspi-config` and enable SPI.
* Add `spidev.bufsiz=65536` to `/etc/cmdline.txt` (in the same line). This
  is the maximum value. The value specified must be at least the block
  length set below. The default `spidev.bufsiz` is 4096.
* Reboot.
* Check `cat /sys/module/spidev/parameters/bufsiz` that it now returns the
  new value.

Only SPI device `/dev/spidev0.1` is available via the GPIO pins.


## Config file: `ola-spidmx.conf`

`enabled = true`  
Enable this plugin (DISABLED by default).

`device_prefix = <string>`  
The prefix of files to match in `/dev`. Usually set to `spidev`. Each match
will instantiate a device.

### Per Device Settings

**Note:** Substitute `<device>` with the full device name, e.g.
`/dev/spidev0.0`.

`<device>-blocklength = 4096`  
How many SPI bytes (= DMX bits) should be received (optional). The default
is 4096, but 8192 is recommended. You may need to make further
configurations on the Raspberry Pi (see above).
