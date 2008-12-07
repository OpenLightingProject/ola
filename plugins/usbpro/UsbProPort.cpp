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

namespace lla {
namespace plugin {

bool UsbProPort::CanRead() const {
  // even ports are input
  return ((PortId()) % 2) == 0;
}

bool UsbProPort::CanWrite() const {
  // odd ports are output
  return (PortId() % 2) == 1;
}


/*
 * Write operation
 *
 * @param  data  pointer to the dmx data
 * @param  length  the length of the data
 *
 * @return   0 on success, non 0 on failure
 */
int UsbProPort::WriteDMX(uint8_t *data, unsigned int length) {
  if (!CanWrite())
    return -1;

  return m_usb_device->SendDmx(data, length);
}


/*
 * Read operation
 *
 * @param   data  buffer to read data into
 * @param   length  length of data to read
 *
 * @return  the amount of data read
 */
int UsbProPort::ReadDMX(uint8_t *data, unsigned int length) {
  if (!CanRead())
    return -1;

  return m_usb_device->FetchDmx(data, length);
}


/*
 * Override SetUniverse.
 * Setting the universe to NULL for an output port will put us back into
 * recv mode.
 */
int UsbProPort::SetUniverse(Universe *uni) {
  Port::SetUniverse(uni);
  if (uni == NULL && CanWrite()) {
    m_usb_device->ChangeToReceiveMode();
  }
  return 0;
}

} // plugin
} //lla
