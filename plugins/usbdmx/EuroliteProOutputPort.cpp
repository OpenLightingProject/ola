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
 * EuroliteProOutputPort.cpp
 * Thread for the EurolitePro Output Port
 * Copyright (C) 2011 Simon Newton & Harry F
 * Eurolite Pro USB DMX ArtNo. 51860120
 */

#include "plugins/usbdmx/EuroliteProOutputPort.h"

#include "ola/Logging.h"
#include "plugins/usbdmx/EuroliteProDevice.h"
#include "plugins/usbdmx/EuroliteProWidget.h"

namespace ola {
namespace plugin {
namespace usbdmx {

EuroliteProOutputPort::EuroliteProOutputPort(EuroliteProDevice *parent,
                                 unsigned int id,
                                 EuroliteProWidget *widget)
    : BasicOutputPort(parent, id),
      m_widget(widget) {
}

EuroliteProOutputPort::~EuroliteProOutputPort() {
  // TODO(simon): stop the thread here??
  OLA_INFO << "EuroliteProOutputPort::~EuroliteProOutputPort()";
}

bool EuroliteProOutputPort::WriteDMX(const DmxBuffer &buffer,
                                     OLA_UNUSED uint8_t priority) {
  m_widget->SendDMX(buffer);
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
