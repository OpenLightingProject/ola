/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * UsbProPort.cpp
 * The USB Pro plugin for lla
 * Copyright (C) 2006-2007 Simon Newton
 */

#include "UsbProPort.h"
#include "UsbProDevice.h"
#include <llad/logger.h>
#include <string.h>


int UsbProPort::can_read() const {
  // even ports are input
  return ( (get_id()+1) % 2);
}

int UsbProPort::can_write() const {
  // odd ports are output
  return ( get_id() % 2 );
}

/*
 * Write operation
 *
 * @param  data  pointer to the dmx data
 * @param  length  the length of the data
 *
 * @return   0 on success, non 0 on failure
 */
int UsbProPort::write(uint8_t *data, unsigned int length) {
  UsbProDevice *dev = (UsbProDevice*) get_device();

  if (!can_write())
    return -1;

  // send to device
  return dev->send_dmx(data, length);
}

/*
 * Read operation
 *
 * @param   data  buffer to read data into
 * @param   length  length of data to read
 *
 * @return  the amount of data read
 */
int UsbProPort::read(uint8_t *data, unsigned int length) {
  UsbProDevice *dev = (UsbProDevice*) get_device();

  if (!can_read())
    return -1;

  // get the device to copy into the buffer
  return dev->get_dmx(data, length);
}

/*
 * Override set_port.
 * Setting the universe to NULL for an output port will put us back into
 * recv mode.
 */
int UsbProPort::set_universe(Universe *uni) {
  UsbProDevice *dev = (UsbProDevice*) get_device();

  Port::set_universe(uni);
  if (uni == NULL && can_write()) {
    dev->recv_mode();
  }
  return 0;
}
