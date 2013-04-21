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
 * SPIDevice.cpp
 * SPI device
 * Copyright (C) 2013 Simon Newton
 */

#include <sstream>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/network/NetworkUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "olad/Universe.h"
#include "plugins/spi/SPIDevice.h"
#include "plugins/spi/SPIPort.h"
#include "plugins/spi/SPIPlugin.h"

namespace ola {
namespace plugin {
namespace spi {

const char SPIDevice::SPI_DEVICE_NAME[] = "SPI Plugin";

/*
 * Create a new device
 */
SPIDevice::SPIDevice(SPIPlugin *owner,
                     Preferences *prefs,
                     PluginAdaptor *plugin_adaptor,
                     const string &spi_device,
                     const UID &uid)
    : Device(owner, SPI_DEVICE_NAME),
      m_preferences(prefs),
      m_plugin_adaptor(plugin_adaptor),
      m_spi_device_name(spi_device) {
  size_t pos = spi_device.find_last_of("/");
  if (pos != string::npos)
    m_spi_device_name = spi_device.substr(pos + 1);

  // 512 / 3 = 170.
  m_preferences->SetDefaultValue(PixelCountKey(), IntValidator(0, 170), "25");
  m_preferences->SetDefaultValue(SPISpeedKey(), IntValidator(0, 32000000),
                                 "100000");

  SPIBackend::Options settings;

  uint8_t pixel_count;
  if (StringToInt(m_preferences->GetValue(PixelCountKey()), &pixel_count)) {
    settings.pixel_count = pixel_count;
  }

  uint32_t spi_speed;
  if (StringToInt(m_preferences->GetValue(SPISpeedKey()), &spi_speed)) {
    settings.spi_speed = spi_speed;
  }

  m_port = new SPIOutputPort(this, spi_device, uid, settings);
}


string SPIDevice::DeviceId() const {
  return m_port->Description();
}


/*
 * Start this device
 */
bool SPIDevice::StartHook() {
  if (!m_port->Init())
    return false;

  uint8_t personality;
  if (StringToInt(m_preferences->GetValue(PersonalityKey()), &personality)) {
    m_port->SetPersonality(personality);
  }

  uint16_t dmx_address;
  if (StringToInt(m_preferences->GetValue(StartAddressKey()), &dmx_address)) {
    m_port->SetStartAddress(dmx_address);
  }

  AddPort(m_port);
  return true;
}


void SPIDevice::PrePortStop() {
  stringstream str;
  str << static_cast<int>(m_port->GetPersonality());
  m_preferences->SetValue(PersonalityKey(), str.str());
  str.str("");
  str << m_port->GetStartAddress();
  m_preferences->SetValue(StartAddressKey(), str.str());
  m_preferences->Save();
}


string SPIDevice::PersonalityKey() const {
  return m_spi_device_name + "-personality";
}

string SPIDevice::StartAddressKey() const {
  return m_spi_device_name + "-dmx-address";
}

string SPIDevice::SPISpeedKey() const {
  return m_spi_device_name + "-spi-speed";
}

string SPIDevice::PixelCountKey() const {
  return m_spi_device_name + "-pixel-count";
}
}  // spi
}  // plugin
}  // ola
