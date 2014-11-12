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
 * SunliteOutputPort.cpp
 * The output port for a Sunlite USBDMX2 device.
 * Copyright (C) 2010 Simon Newton
 */

#include "plugins/usbdmx/SunliteOutputPort.h"

#include "ola/Logging.h"
#include "plugins/usbdmx/SunliteDevice.h"
#include "plugins/usbdmx/SunliteWidget.h"

namespace ola {
namespace plugin {
namespace usbdmx {

SunliteOutputPort::SunliteOutputPort(SunliteDevice *parent,
                                     unsigned int id,
                                     SunliteWidget *widget)
    : BasicOutputPort(parent, id),
      m_widget(widget) {
}

SunliteOutputPort::~SunliteOutputPort() {
  // TODO(simon): stop the thread here??
  OLA_INFO << "SunliteOutputPort::~SunliteOutputPort()";
  delete m_widget;
}

bool SunliteOutputPort::WriteDMX(const DmxBuffer &buffer,
                                 OLA_UNUSED uint8_t priority) {
  m_widget->SendDMX(buffer);
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
