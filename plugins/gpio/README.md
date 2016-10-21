General Purpose I/O Plugin
==========================

This plugin controls the General Purpose Digital I/O (GPIO) pins on devices like
a Raspberry Pi. It creates a single device, with a single output port. The
offset (start address) of the GPIO pins is configurable.


## Config file: `ola-gpio.conf`

`gpio_pins = [int]`  
The list of GPIO pins to control, each pin is mapped to a DMX512 slot.

`gpio_slot_offset = <int>`  
The DMX512 slot for the first pin. Slots are indexed from 1.

`gpio_turn_on = <int>`  
The DMX512 value above which a GPIO pin will be turned on.

`gpio_turn_off = <int>`  
The DMX512 value below which a GPIO pin will be turned off.
