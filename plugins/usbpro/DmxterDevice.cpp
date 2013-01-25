/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * DmxterDevice.h
 * A Goddard Design Dmxter.
 * Copyright (C) 2010 Simon Newton
 */

#include <algorithm>
#include <string>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/DmxterDevice.h"
#include "plugins/usbpro/DmxterWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::string;
using ola::rdm::UID;


/*
 * New DMXter device
 */
DmxterDevice::DmxterDevice(ola::AbstractPlugin *owner,
                           const string &name,
                           DmxterWidget *widget,
                           uint16_t esta_id,
                           uint16_t device_id,
                           uint32_t serial):
    UsbSerialDevice(owner, name, widget) {
  std::stringstream str;
  str << std::hex << esta_id << "-" << device_id << "-" << serial;
  m_device_id = str.str();

  ola::BasicOutputPort *port = new DmxterOutputPort(this, widget);
  AddPort(port);
}
}  // usbpro
}  // plugin
}  // ola
