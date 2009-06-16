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

#include <lla/Logging.h>
#include "UsbProPort.h"
#include "UsbProDevice.h"

namespace lla {
namespace usbpro {

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
 * @param data  pointer to the dmx data
 * @param length  the length of the data
 * @return true on success, false on failure
 */
bool UsbProPort::WriteDMX(const DmxBuffer &buffer) {
  if (!CanWrite())
    return true;
  return GetDevice()->SendDMX(buffer);
}


/*
 * Read operation
 * @param data  buffer to read data into
 * @param length length of data to read
 * @return the amount of data read
 */
const DmxBuffer &UsbProPort::ReadDMX() const {
  if (!CanRead())
    return m_empty_buffer;
  return GetDevice()->FetchDMX();
}


/*
 * Override SetUniverse.
 * Setting the universe to NULL for an output port will put us back into
 * recv mode.
 */
bool UsbProPort::SetUniverse(Universe *uni) {
  Port<UsbProDevice>::SetUniverse(uni);
  if (uni == NULL && CanWrite()) {
    GetDevice()->ChangeToReceiveMode();
  }
  return 0;
}

/*
 * return the unique port id
 */
string UsbProPort::UniqueId() const {
  AbstractPlugin *plugin = GetDevice()->Owner();
  if (!plugin)
    return "";

  std::stringstream str;
  str << plugin->Id() << "-" << GetDevice()->SerialNumber() << "-" << PortId();
  LLA_INFO << "uniq is " << GetDevice()->SerialNumber();
  return str.str();
}

} // usbpro
} //lla
