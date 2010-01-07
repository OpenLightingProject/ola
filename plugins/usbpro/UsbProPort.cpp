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
 * The USB Pro plugin for ola
 * Copyright (C) 2006-2007 Simon Newton
 */

#include <ola/Logging.h>
#include "plugins/usbpro/UsbProPort.h"
#include "plugins/usbpro/UsbProDevice.h"

namespace ola {
namespace plugin {
namespace usbpro {

/*
 * Read operation
 * @param data  buffer to read data into
 * @param length length of data to read
 * @return the amount of data read
 */
const DmxBuffer &UsbProInputPort::ReadDMX() const {
  return m_device->FetchDMX();
}


/*
 * Override SetUniverse.
 * Setting the universe to NULL for an output port will put us back into
 * recv mode.
 */
void UsbProOutputPort::PostSetUniverse(Universe *new_universe,
                                       Universe *old_universe) {
  if (!new_universe)
    m_device->ChangeToReceiveMode();
}


/*
 * Write operation
 * @param data  pointer to the dmx data
 * @param length  the length of the data
 * @return true on success, false on failure
 */
bool UsbProOutputPort::WriteDMX(const DmxBuffer &buffer,
                                uint8_t priority) {
  return m_device->SendDMX(buffer);
}
}  // usbpro
}  // plugin
}  // ola
