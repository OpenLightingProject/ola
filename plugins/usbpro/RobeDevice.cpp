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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * RobeDevice.cpp
 * A Robe Universal Interface device.
 * Copyright (C) 2011 Simon Newton
 */

#include <algorithm>
#include <iomanip>
#include <string>
#include "ola/Constants.h"
#include "ola/io/SelectServerInterface.h"
#include "plugins/usbpro/RobeDevice.h"
#include "plugins/usbpro/RobeWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::string;

/*
 * New RobeDevice.
 */
RobeDevice::RobeDevice(ola::PluginAdaptor *plugin_adaptor,
                       ola::AbstractPlugin *owner,
                       const string &name,
                       RobeWidget *widget)
    : UsbSerialDevice(owner, name, widget) {
  std::ostringstream str;
  str << 1;
  m_device_id = str.str();

  m_output_port = new RobeOutputPort(this, widget);
  AddPort(m_output_port);
  m_input_port = new RobeInputPort(this, widget, plugin_adaptor);
  AddPort(m_input_port);
}


/**
 * Create a new Robe Output Port
 */
RobeOutputPort::RobeOutputPort(RobeDevice *parent,
                               RobeWidget *widget)
    : BasicOutputPort(parent, 0, true, true),
      m_widget(widget) {
}


/**
 * Write DMX to the output port
 */
bool RobeOutputPort::WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
  m_widget->SendDMX(buffer);
  (void) priority;
  return true;
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
