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
 * DMXCProjectsNodleU1Port.cpp
 * Output and input ports that use a widget and give the serial as desciption.
 * Copyright (C) 2015 Stefan Krupop
 */

#include "plugins/usbdmx/DMXCProjectsNodleU1Port.h"

#include "ola/Logging.h"
#include "olad/Device.h"

namespace ola {
namespace plugin {
namespace usbdmx {

DMXCProjectsNodleU1InputPort::DMXCProjectsNodleU1InputPort(
    Device *parent,
    unsigned int id,
    PluginAdaptor *plugin_adaptor,
    DMXCProjectsNodleU1 *widget)
    : BasicInputPort(parent, id, plugin_adaptor),
      m_widget(widget) {
  m_widget->SetDmxCallback(NewCallback(
      static_cast<BasicInputPort*>(this),
      &BasicInputPort::DmxChanged));
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
