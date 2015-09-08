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
 * NodleU1Port.h
 * Output and input ports that use a widget and give the serial as desciption.
 * Copyright (C) 2015 Stefan Krupop
 */

#ifndef PLUGINS_USBDMX_NODLEU1PORT_H_
#define PLUGINS_USBDMX_NODLEU1PORT_H_

#include <string>
#include <iostream>
#include "ola/base/Macro.h"
#include "olad/Port.h"
#include "plugins/usbdmx/NodleU1.h"

namespace ola {

class Device;

namespace plugin {
namespace usbdmx {

/**
 * @brief A thin wrapper around a Widget so that it can operate as a Port.
 */
class NodleU1OutputPort: public BasicOutputPort {
 public:
  /**
   * @brief Create a new NodleU1OutputPort.
   * @param parent The parent device for this port.
   * @param id The port id.
   * @param widget The widget to use to send DMX frames.
   */
  NodleU1OutputPort(Device *parent,
                    unsigned int id,
                    class NodleU1 *widget);

  bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

  std::string Description() const {
    std::ostringstream str;
    str << "Serial #: " << m_widget->SerialNumber();
    return str.str();
  }

 private:
  class NodleU1* const m_widget;

  DISALLOW_COPY_AND_ASSIGN(NodleU1OutputPort);
};

/**
 * @brief A thin wrapper around a Widget so that it can operate as a Port.
 */
class NodleU1InputPort: public BasicInputPort {
 public:
  /**
   * @brief Create a new NodleU1InputPort.
   * @param parent The parent device for this port.
   * @param id The port id.
   * @param plugin_adaptor a PluginAdaptor object, used by the base class.
   * @param widget The widget to use to receive DMX frames.
   */
  NodleU1InputPort(Device *parent,
                   unsigned int id,
                   PluginAdaptor *plugin_adaptor,
                   class NodleU1 *widget);

  const DmxBuffer &ReadDMX() const {
    return m_widget->GetDmxInBuffer();
  }

  std::string Description() const {
    std::ostringstream str;
    str << "Serial #: " << m_widget->SerialNumber();
    return str.str();
  }

 private:
  DmxBuffer m_buffer;
  class NodleU1* const m_widget;

  DISALLOW_COPY_AND_ASSIGN(NodleU1InputPort);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_NODLEU1PORT_H_
