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

#include <string>
#include <stdio.h>

#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/uartdmx/UartDmxDevice.h"
#include "plugins/uartdmx/UartWidget.h"
#include "plugins/uartdmx/UartDmxInputThread.h"
#include "plugins/uartdmx/UartDmxOutputThread.h"
#include "plugins/uartdmx/UartDmxPort.h"

namespace ola {
namespace plugin {
namespace uartdmx {

UartDmxInputPort::UartDmxInputPort(
    UartDmxDevice *parent,
    unsigned int port_id,
    PluginAdaptor *plugin_adaptor,
    UartWidget *widget)
    : BasicInputPort(parent, port_id, plugin_adaptor),
      m_port_id(port_id),
      m_widget(widget),
      m_thread(widget, this) {
  m_thread.Start();
}

UartDmxInputPort::~UartDmxInputPort() {
  printf("UartDmxInputPort destructor called\n");
  m_thread.Stop();
}

void UartDmxInputPort::NewDMXData(const DmxBuffer &data) {
  m_buffer.Set(data);  // store the data
  DmxChanged();  // signal that our data has changed
}


UartDmxOutputPort::UartDmxOutputPort(
    UartDmxDevice *parent,
    unsigned int id,
    UartWidget *widget,
    unsigned int breakt,
    unsigned int malft)
    : BasicOutputPort(parent, id),
      m_widget(widget),
      m_thread(widget, breakt, malft) {
  m_thread.Start();
}

UartDmxOutputPort::~UartDmxOutputPort() {
  m_thread.Stop();
}

}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
