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
 * UartDmxPort.h
 * The DMX through a UART plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#ifndef PLUGINS_UARTDMX_UARTDMXPORT_H_
#define PLUGINS_UARTDMX_UARTDMXPORT_H_

#include <string>

#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "olad/Preferences.h"
#include "plugins/uartdmx/UartDmxDevice.h"
#include "plugins/uartdmx/UartWidget.h"
#include "plugins/uartdmx/UartDmxThread.h"

namespace ola {
namespace plugin {
namespace uartdmx {

class UartDmxOutputPort : public ola::BasicOutputPort {
 public:
  UartDmxOutputPort(UartDmxDevice *parent,
                    unsigned int id,
                    UartWidget *widget,
                    unsigned int breakt,
                    unsigned int malft)
      : BasicOutputPort(parent, id),
        m_widget(widget),
        m_thread(widget, breakt, malft) {
    m_thread.Start();
  }
  ~UartDmxOutputPort() { m_thread.Stop(); }

  bool WriteDMX(const ola::DmxBuffer &buffer, uint8_t) {
    return m_thread.WriteDMX(buffer);
  }

  std::string Description() const { return m_widget->Description(); }

 private:
  UartWidget *m_widget;
  UartDmxThread m_thread;

  DISALLOW_COPY_AND_ASSIGN(UartDmxOutputPort);
};
}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_UARTDMX_UARTDMXPORT_H_
