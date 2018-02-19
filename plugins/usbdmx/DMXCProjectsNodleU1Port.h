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
 * DMXCProjectsNodleU1Port.h
 * Output and input ports that use a widget and give the serial as description.
 * Copyright (C) 2015 Stefan Krupop
 */

#ifndef PLUGINS_USBDMX_DMXCPROJECTSNODLEU1PORT_H_
#define PLUGINS_USBDMX_DMXCPROJECTSNODLEU1PORT_H_

#include <string>
#include <iostream>
#include "ola/base/Macro.h"
#include "olad/Port.h"
#include "plugins/usbdmx/DMXCProjectsNodleU1.h"

namespace ola {

class Device;

namespace plugin {
namespace usbdmx {

/**
 * @brief A thin wrapper around a Widget so that it can operate as a Port.
 */
class DMXCProjectsNodleU1InputPort: public BasicInputPort {
 public:
  /**
   * @brief Create a new DMXCProjectsNodleU1InputPort.
   * @param parent The parent device for this port.
   * @param id The port id.
   * @param plugin_adaptor a PluginAdaptor object, used by the base class.
   * @param widget The widget to use to receive DMX frames.
   */
  DMXCProjectsNodleU1InputPort(Device *parent,
                               unsigned int id,
                               PluginAdaptor *plugin_adaptor,
                               class DMXCProjectsNodleU1 *widget);

  const DmxBuffer &ReadDMX() const {
    return m_widget->GetDmxInBuffer();
  }

  std::string Description() const {
    return "";
  }

 private:
  class DMXCProjectsNodleU1* const m_widget;

  DISALLOW_COPY_AND_ASSIGN(DMXCProjectsNodleU1InputPort);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_DMXCPROJECTSNODLEU1PORT_H_
