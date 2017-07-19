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
 * SpiDmxDevice.cpp
 * The DMX through a SPI plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#include <string>
#include <memory>
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "plugins/spidmx/SpiDmxDevice.h"
//#include "plugins/spidmx/SpiDmxPort.h"

namespace ola {
namespace plugin {
namespace spidmx {

using std::string;

const unsigned int SpiDmxDevice::DEFAULT_BLOCKLENGTH = 4096;
const char SpiDmxDevice::BLOCKLENGTH_KEY[] = "-blocklength";


SpiDmxDevice::SpiDmxDevice(AbstractPlugin *owner,
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
  if (!StringToInt(m_preferences->GetValue(DeviceBlocklength()),
                                           &m_blocklength)) {
    m_blocklength = DEFAULT_BLOCKLENGTH;
  }
  // m_widget.reset(new SpiWidget(path));
  OLA_DEBUG << "SpiDmxDevice constructor called";
}

SpiDmxDevice::~SpiDmxDevice() {
  /*if (m_widget->IsOpen()) {
    m_widget->Close();
  }*/
  OLA_DEBUG << "SpiDmxDevice destructor called";
}

bool SpiDmxDevice::StartHook() {
  // AddPort(new SpiDmxOutputPort(this, 0, m_widget.get(), m_breakt, m_malft));
  OLA_DEBUG << "SpiDmxDevice::StartHook called";
  return true;
}

string SpiDmxDevice::DeviceBlocklength() const {
  return m_path + BLOCKLENGTH_KEY;
}

/**
 * Set the default preferences for this one Device
 */
void SpiDmxDevice::SetDefaults() {
  if (!m_preferences) {
    return;
  }

  bool save = false;

  save |= m_preferences->SetDefaultValue(DeviceBlocklength(),
                                         UIntValidator(0, 65535),
                                         DEFAULT_BLOCKLENGTH);
  if (save) {
    m_preferences->Save();
  }
}
}  // namespace spidmx
}  // namespace plugin
}  // namespace ola
