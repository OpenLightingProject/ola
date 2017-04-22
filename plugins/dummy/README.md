Dummy Plugin
============

The plugin creates a single device with one port. When used as an output
port it prints the first two bytes of dmx data to stdout.

The Dummy plugin can also emulate a range of RDM devices. It supports the
following RDM device types:

* Dummy Device (original)
* Dimmer Rack, with a configurable number of sub-devices
* Moving Light
* Advanced Dimmer Rack, with E1.37-1 PIDs
* A device that responds with ack timers
* Sensor Device, with a number of sensors implemented
* Network Device, with E1.37-2 PIDs

The number of each type of device is configurable.


## Config file: `ola-dummy.conf`

`ack_timer_count = 0`  
The number of ack timer responders to create.

`advanced_dimmer_count = 0`  
The number of E1.37-1 dimmer responders to create.

`dimmer_count = 1`  
The number of dimmer devices to create.

`dimmer_subdevice_count = 1`  
The number of sub-devices each dimmer device should have.

`dummy_device_count = 1`  
The number of dummy devices to create.

`moving_light_count = 1`  
The number of moving light devices to create.

`sensor_device_count = 1`  
The number of sensor-only devices to create.

`network_device_count = 1`  
The number of network E1.37-2 devices to create.
