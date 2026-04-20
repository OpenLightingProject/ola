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
 * DMXCProjectsNodleU1Device.cpp
 * A DMXCProjectsNodleU1 device that creates an input and an output port.
 * Copyright (C) 2015 Stefan Krupop
 */

#include "plugins/usbdmx/DMXCProjectsNodleU1Device.h"

#include <string>
#include "plugins/usbdmx/DMXCProjectsNodleU1Port.h"
#include "plugins/usbdmx/GenericOutputPort.h"

namespace ola {
namespace plugin {
namespace usbdmx {

DMXCProjectsNodleU1Device::DMXCProjectsNodleU1Device(
    ola::AbstractPlugin *owner,
    DMXCProjectsNodleU1 *widget,
    const std::string &device_name,
    const std::string &device_id,
    PluginAdaptor *plugin_adaptor)
    : Device(owner, device_name),
      m_device_id(device_id),
      m_widget(widget),
      m_plugin_adaptor(plugin_adaptor) {
}

bool DMXCProjectsNodleU1Device::StartHook() {
  unsigned int mode = m_widget->Mode();
  unsigned int ins = m_widget->Ins();
  unsigned int outs = m_widget->Outs();

  bool ok = true;

  OLA_DEBUG << "StartHook, will create " << ins << " INs and " << outs << " OUTs";

  if (ins != 1) {
    for (unsigned i = 0; i < ins; i++) {
      OLA_DEBUG << "IN " << i;
      DMXCProjectsNodleU1InputPort *port = new DMXCProjectsNodleU1InputPort(
                                                 this, i, m_plugin_adaptor,
                                                 m_widget); 
      if (!AddPort(port)) {
        OLA_DEBUG << "FAILED";
        delete port;
        ok = false;
      }
    }
  } else if (mode & DMXCProjectsNodleU1::INPUT_ENABLE_MASK) {
    // input port active
    DMXCProjectsNodleU1InputPort *port = new DMXCProjectsNodleU1InputPort(
                                              this, 0, m_plugin_adaptor,
                                              m_widget); 
    if (!AddPort(port)) {
      delete port;
      ok = false;
    }
  }

  if (outs != 1) {
    for (unsigned i = 0; i < outs; i++) {
      OLA_DEBUG << "OUT " << i;
      GenericOutputPort *port = new GenericOutputPort(this, i, m_widget);
      if (!AddPort(port)) {
        OLA_DEBUG << "FAILED";
        delete port;
        ok = false;
      }
    }
  } else if (mode & DMXCProjectsNodleU1::OUTPUT_ENABLE_MASK) {
    // output port active
    GenericOutputPort *port = new GenericOutputPort(this, 0, m_widget);
    if (!AddPort(port)) {
      delete port;
      ok = false;
    }
  }

  OLA_DEBUG << "StartHook will return " << ok;
  return ok;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
