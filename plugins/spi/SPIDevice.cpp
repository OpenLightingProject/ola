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
const char SPIDevice::HARDWARE_BACKEND[] = "hardware_multiplexer";
const char SPIDevice::MERGED_BACKEND[] = "software_multiplexer";

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

  SetDefaults();

  uint8_t port_count = 1;
  StringToInt(m_preferences->GetValue(PortCountKey()), &pixel_count);

  string backend_type;
  m_preferences->GetValue(SPIBackendKey(), &backend_type);
  if (backend_type == HARDWARE_BACKEND) {
    MultiplexedSPIBackend::Options options;
    PopulateMergedBackendOptions(&options);
    m_backend.reset(new MultiplexedSPIBackend(spi_device, options));

  } else {
    if (backend_type != MERGED_BACKEND) {
      OLA_WARN << "Unknown backend_type '" << backend_type
               << "' for SPI device " << m_spi_device_name;
    }

    ChainedSPIBackend::Options options;
    PopulateChainedBackendOptions(&options);
    options.outputs = port_count;
    m_backend.reset(new ChainedSPIBackend(spi_device, options));
  }

  for (uint8_t i = 0; i < port_count; i++) {
    SPIOutput::Options spi_output_options;
    spi_output_options.output_number = i;

    uint8_t pixel_count;
    if (StringToInt(m_preferences->GetValue(PixelCountKey()), &pixel_count)) {
      spi_output_options.pixel_count = pixel_count;
    }

    m_spi_ports.push_back(
        new SPIOutputPort(this, m_backend.get(), uid, spi_output_options));
  }
}


string SPIDevice::DeviceId() const {
  return m_port->Description();
}


/*
 * Start this device
 */
bool SPIDevice::StartHook() {
  if (!m_backend->Init()) {
    return false;
  }

  SPIPorts::iterator iter = m_spi_ports.begin();
  for (; iter != m_spi_ports.end(); iter++) {
    uint8_t personality;
    if (StringToInt(m_preferences->GetValue(PersonalityKey()), &personality)) {
      (*iter)->SetPersonality(personality);
    }

    uint16_t dmx_address;
    if (StringToInt(m_preferences->GetValue(StartAddressKey()), &dmx_address)) {
      (*iter)->SetStartAddress(dmx_address);
    }

    AddPort(*iter);
  }
  return true;
}


void SPIDevice::PrePortStop() {
  SPIPorts::iterator iter = m_spi_ports.begin();
  for (; iter != m_spi_ports.end(); iter++) {
    stringstream str;
    str << static_cast<int>((*iter)->GetPersonality());
    m_preferences->SetValue(PersonalityKey(), str.str());
    str.str("");
    str << (*iter)->GetStartAddress();
    m_preferences->SetValue(StartAddressKey(), str.str());
  }
  m_preferences->Save();
}

string SPIDevice::SPIBackendKey() const {
  return m_spi_device_name + "-backend";
}

string SPIDevice::SPISpeedKey() const {
  return m_spi_device_name + "-spi-speed";
}

string SPIDevice::PortCountKey() const {
  return m_spi_device_name + "-ports";
}

string SPIDevice::PersonalityKey() const {
  return m_spi_device_name + "-personality";
}

string SPIDevice::StartAddressKey() const {
  return m_spi_device_name + "-dmx-address";
}

string SPIDevice::PixelCountKey() const {
  return m_spi_device_name + "-pixel-count";
}

void SPIDevice::SetDefaults() {
  set<string> valid_backends;
  valid_backends.insert(HARDWARE_BACKEND);
  valid_backends.insert(MERGED_BACKEND);
  m_preferences->SetDefaultValue(SPIBackendKey(), SetValidator(valid_backends),
                                 "MERGED_BACKEND");

  m_preferences->SetDefaultValue(PortCountKey(), IntValidator(1, 8), "1");
  // 512 / 3 = 170.
  m_preferences->SetDefaultValue(PixelCountKey(), IntValidator(0, 170), "25");
  m_preferences->SetDefaultValue(SPISpeedKey(), IntValidator(0, 32000000),
                                 "100000");
}

void SPIDevice::PopulateMultipliexerBackendOptions(
    MultiplexedSPIBackend::Options *options) {
  PopulateOptions(options);

  // add gpio pins here
}

void SPIDevice::PopulateChainedBackendOptions(
    ChainedSPIBackend::Options *options) {
  PopulateOptions(options);

}

void SPIDevice::PopulateOptions(SPIBackend::Options *options) {
  uint32_t spi_speed;
  if (StringToInt(m_preferences->GetValue(SPISpeedKey()), &spi_speed)) {
    options->spi_speed = spi_speed;
  }
}
}  // namespace spi
}  // namespace plugin
}  // namespace ola
