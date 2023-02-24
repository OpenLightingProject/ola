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
 * GenericOutputPort.h
 * A Generic output port that uses a widget.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_GENERICOUTPUTPORT_H_
#define PLUGINS_USBDMX_GENERICOUTPUTPORT_H_

#include <string>
#include "ola/base/Macro.h"
#include "olad/Port.h"

namespace ola {

class Device;

namespace plugin {
namespace usbdmx {

class Widget;

/**
 * @brief A thin wrapper around a Widget so that it can operate as a Port.
 */
class GenericOutputPort: public BasicOutputPort {
 public:
  /**
   * @brief Create a new GenericOutputPort.
   * @param parent The parent device for this port.
   * @param id The port id.
   * @param widget The widget to use to send DMX frames.
   */
  GenericOutputPort(Device *parent,
                    unsigned int id,
                    class WidgetInterface *widget);

  bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

  std::string Description() const { return ""; }

 private:
  class WidgetInterface* const m_widget;

  DISALLOW_COPY_AND_ASSIGN(GenericOutputPort);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_GENERICOUTPUTPORT_H_
