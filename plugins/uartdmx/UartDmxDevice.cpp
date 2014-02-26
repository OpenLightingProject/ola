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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * UartDmxDevice.cpp
 * The DMX through a UART plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#include <string>
#include <memory>
#include "ola/Logging.h"
#include "plugins/uartdmx/UartDmxDevice.h"
#include "plugins/uartdmx/UartDmxPort.h"

namespace ola {
namespace plugin {
namespace uartdmx {

using std::string;

UartDmxDevice::UartDmxDevice(AbstractPlugin *owner,
                             const std::string &name,
                             const std::string &path,
                             unsigned int device_id,
                             unsigned int breakt,
                             unsigned int malft)
    : Device(owner, name),
      m_name(name),
      m_path(path),
      m_device_id(device_id),
      m_breakt(breakt),
      m_malft(malft){
  m_widget.reset(
      new UartWidget(path, device_id));
}

UartDmxDevice::~UartDmxDevice() {
  if (m_widget->IsOpen())
    m_widget->Close();
}

bool UartDmxDevice::StartHook() {
  AddPort(new UartDmxOutputPort(this,
                                m_widget.get(),
                                m_device_id,
                                m_breakt,
                                m_malft));
  return true;
}
}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
