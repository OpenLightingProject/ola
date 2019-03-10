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
 * FtdiDmxDevice.cpp
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 *
 * Additional modifications to enable support for multiple outputs and
 * additional device ids did change the original structure.
 *
 * by E.S. Rosenberg a.k.a. Keeper of the Keys 5774/2014
 */

#include <string>
#include <memory>
#include "ola/Logging.h"
#include "plugins/ftdidmx/FtdiDmxDevice.h"
#include "plugins/ftdidmx/FtdiDmxPort.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

using std::string;

FtdiDmxDevice::FtdiDmxDevice(AbstractPlugin *owner,
                             const FtdiWidgetInfo &widget_info,
                             unsigned int frequency)
    : Device(owner, widget_info.Description()),
      m_widget_info(widget_info),
      m_frequency(frequency) {
  m_widget = new FtdiWidget(widget_info.Serial(),
                            widget_info.Name(),
                            widget_info.Id(),
                            widget_info.Vid(),
                            widget_info.Pid());
}

FtdiDmxDevice::~FtdiDmxDevice() {
  DeleteAllPorts();
  delete m_widget;
}

bool FtdiDmxDevice::StartHook() {
  unsigned int interface_count = m_widget->GetInterfaceCount();
  unsigned int successfully_added = 0;
  unsigned int serial = 0;

  OLA_INFO << "Widget " << m_widget->Name() << " has " << interface_count
           << " interfaces.";

  for (unsigned int i = 1; i <= interface_count; i++) {
    if (!StringToInt(m_widget->Serial(), &serial, false, 36)) {
      OLA_WARN << "StringToInt returned false, serial used: " << serial;
    }
    FtdiInterface *port = new FtdiInterface(m_widget,
                                            static_cast<ftdi_interface>(i));
    if (port->SetupOutput()) {
      AddPort(new FtdiDmxOutputPort(this, port, i, m_frequency, serial));
      successfully_added += 1;
    } else {
      OLA_WARN << "Failed to add interface: " << i;
      delete port;
    }
  }
  if (successfully_added > 0) {
    OLA_INFO << "Successfully added " << successfully_added << "/"
             << interface_count << " interfaces.";
  } else {
    OLA_INFO << "Removing widget since no ports were added.";
    return false;
  }

  return true;
}
}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
