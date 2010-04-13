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
 * DmxTriDevice.h
 * The Jese DMX TRI device.
 * Copyright (C) 2010 Simon Newton
 */

#include <algorithm>
#include <string>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/usbpro/DmxTriDevice.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::string;
using ola::network::NetworkToHost;


/*
 * New Arduino RGB Device
 */
DmxTriDevice::DmxTriDevice(ola::AbstractPlugin *owner,
                                   const string &name,
                                   UsbWidget *widget,
                                   uint16_t esta_id,
                                   uint16_t device_id,
                                   uint32_t serial):
    UsbDevice(owner, name, widget) {
  std::stringstream str;
  str << std::hex << esta_id << "-" << device_id << "-" <<
    NetworkToHost(serial);
  m_device_id = str.str();
  DmxTriOutputPort *output_port = new DmxTriOutputPort(this);
  AddPort(output_port);
  Start();
}


/*
 * Send a dmx msg
 * @returns true if we sent ok, false otherwise
 */
bool DmxTriDevice::SendDMX(const DmxBuffer &buffer) const {
  struct {
    uint8_t start_code;
    uint8_t dmx[DMX_UNIVERSE_SIZE];
  } widget_dmx;

  if (!IsEnabled())
    return true;

  widget_dmx.start_code = 0;
  unsigned int length = DMX_UNIVERSE_SIZE;
  buffer.Get(widget_dmx.dmx, &length);
  return m_widget->SendMessage(UsbWidget::DMX_LABEL,
                               length + 1,
                               reinterpret_cast<uint8_t*>(&widget_dmx));
}
}  // usbpro
}  // plugin
}  // ola
