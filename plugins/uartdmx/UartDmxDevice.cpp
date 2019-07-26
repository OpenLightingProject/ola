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
 * UartDmxDevice.cpp
 * The DMX through a UART plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#include <string>
#include <memory>
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "plugins/uartdmx/UartDmxDevice.h"
#include "plugins/uartdmx/UartDmxPort.h"

namespace ola {
namespace plugin {
namespace uartdmx {

using std::string;

const char UartDmxDevice::K_MALF[] = "-malf";
const char UartDmxDevice::K_BREAK[] = "-break";
const char UartDmxDevice::K_PADDING[] = "-padding";
const unsigned int UartDmxDevice::DEFAULT_BREAK = 100;
const unsigned int UartDmxDevice::DEFAULT_MALF = 100;
const unsigned int UartDmxDevice::DEFAULT_PADDING = 24;


UartDmxDevice::UartDmxDevice(AbstractPlugin *owner,
                             class Preferences *preferences,
                             const string &name,
                             const string &path)
    : Device(owner, name),
      m_preferences(preferences),
      m_name(name),
      m_path(path) {
  // set up some per-device default configuration if not already set
  SetDefaults();
  // now read per-device configuration
  // Break time in microseconds
  if (!StringToInt(m_preferences->GetValue(DeviceBreakKey()), &m_breakt)) {
    m_breakt = DEFAULT_BREAK;
  }
  // Mark After Last Frame in microseconds
  if (!StringToInt(m_preferences->GetValue(DeviceMalfKey()), &m_malft)) {
    m_malft = DEFAULT_MALF;
  }
  // Minimum amount of DMX channels to transmit
  if (!StringToInt(m_preferences->GetValue(DevicePaddingKey()), &m_padding)) {
    m_padding = DEFAULT_PADDING;
  }
  m_widget.reset(new UartWidget(path, m_padding));
}

UartDmxDevice::~UartDmxDevice() {
  if (m_widget->IsOpen()) {
    m_widget->Close();
  }
}

bool UartDmxDevice::StartHook() {
  AddPort(new UartDmxOutputPort(this, 0, m_widget.get(), m_breakt, m_malft));
  return true;
}

string UartDmxDevice::DeviceMalfKey() const {
  return m_path + K_MALF;
}
string UartDmxDevice::DeviceBreakKey() const {
  return m_path + K_BREAK;
}
string UartDmxDevice::DevicePaddingKey() const {
  return m_path + K_PADDING;
}

/**
 * Set the default preferences for this one Device
 */
void UartDmxDevice::SetDefaults() {
  if (!m_preferences) {
    return;
  }

  bool save = false;

  save |= m_preferences->SetDefaultValue(DeviceBreakKey(),
                                         UIntValidator(88, 1000000),
                                         DEFAULT_BREAK);
  save |= m_preferences->SetDefaultValue(DeviceMalfKey(),
                                         UIntValidator(8, 1000000),
                                         DEFAULT_MALF);
  save |= m_preferences->SetDefaultValue(DevicePaddingKey(),
                                         UIntValidator(24, 512),
                                         DEFAULT_PADDING);
  if (save) {
    m_preferences->Save();
  }
}
}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
