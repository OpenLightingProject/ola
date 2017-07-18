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
 * SpiDevice.cpp
 * SPI device
 * Copyright (C) 2013 Simon Newton
 */

#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/file/Util.h"
#include "ola/network/NetworkUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "olad/Universe.h"
#include "plugins/spi/SpiDevice.h"
#include "plugins/spi/SpiPort.h"
#include "plugins/spi/SpiPlugin.h"

namespace ola {
namespace plugin {
namespace spi {

using ola::rdm::UID;
using std::auto_ptr;
using std::ostringstream;
using std::set;
using std::string;
using std::vector;

const char SpiDevice::SPI_DEVICE_NAME[] = "SPI Device";
const char SpiDevice::HARDWARE_BACKEND[] = "hardware";
const char SpiDevice::SOFTWARE_BACKEND[] = "software";

/*
 * Create a new device
 */
SpiDevice::SpiDevice(SpiPlugin *owner,
                     Preferences *prefs,
                     PluginAdaptor *plugin_adaptor,
                     const string &spi_device,
                     ola::rdm::UIDAllocator *uid_allocator)
    : Device(owner, SPI_DEVICE_NAME),
      m_preferences(prefs),
      m_plugin_adaptor(plugin_adaptor),
      m_spi_device_name(spi_device) {
  m_spi_device_name = ola::file::FilenameFromPathOrPath(m_spi_device_name);

  ostringstream str;
  str << SPI_DEVICE_NAME << " " << m_spi_device_name;
  SetName(str.str());

  SetDefaults();
  unsigned int port_count = 0;

  string backend_type = m_preferences->GetValue(SpiBackendKey());
  SpiWriter::Options writer_options;
  PopulateWriterOptions(&writer_options);
  m_writer.reset(new SpiWriter(spi_device, writer_options,
                               plugin_adaptor->GetExportMap()));


  if (backend_type == HARDWARE_BACKEND) {
    HardwareBackend::Options options;
    PopulateHardwareBackendOptions(&options);
    m_backend.reset(
        new HardwareBackend(options, m_writer.get(),
                            plugin_adaptor->GetExportMap()));
    port_count = 1 << options.gpio_pins.size();
    OLA_INFO << m_spi_device_name << ", Hardware backend, " << port_count
             << " ports";
  } else {
    if (backend_type != SOFTWARE_BACKEND) {
      OLA_WARN << "Unknown backend_type '" << backend_type
               << "' for SPI device " << m_spi_device_name;
    }

    SoftwareBackend::Options options;
    PopulateSoftwareBackendOptions(&options);
    m_backend.reset(
        new SoftwareBackend(options, m_writer.get(),
                            plugin_adaptor->GetExportMap()));
    port_count = options.outputs;
    OLA_INFO << m_spi_device_name << ", Software backend, " << port_count
             << " ports";
  }

  for (uint8_t i = 0; i < port_count; i++) {
    SpiOutput::Options spi_output_options(i, m_spi_device_name);

    if (m_preferences->HasKey(DeviceLabelKey(i))) {
      spi_output_options.device_label =
          m_preferences->GetValue(DeviceLabelKey(i));
    }

    uint8_t pixel_count;
    if (StringToInt(m_preferences->GetValue(PixelCountKey(i)), &pixel_count)) {
      spi_output_options.pixel_count = pixel_count;
    }

    auto_ptr<UID> uid(uid_allocator->AllocateNext());
    if (!uid.get()) {
      OLA_WARN << "Insufficient UIDs remaining to allocate a UID for SPI port "
               << static_cast<int>(i);
      continue;
    }

    m_spi_ports.push_back(
        new SpiOutputPort(this, m_backend.get(), *uid.get(),
                          spi_output_options));
  }
}


string SpiDevice::DeviceId() const {
  return m_spi_device_name;
}


/*
 * Start this device
 */
bool SpiDevice::StartHook() {
  if (!m_backend->Init()) {
    STLDeleteElements(&m_spi_ports);
    return false;
  }

  SpiPorts::iterator iter = m_spi_ports.begin();
  for (uint8_t i = 0; iter != m_spi_ports.end(); iter++, i++) {
    uint8_t personality;
    if (StringToInt(m_preferences->GetValue(PersonalityKey(i)),
                    &personality)) {
      (*iter)->SetPersonality(personality);
    }

    uint16_t dmx_address;
    if (StringToInt(m_preferences->GetValue(StartAddressKey(i)),
                                            &dmx_address)) {
      (*iter)->SetStartAddress(dmx_address);
    }

    AddPort(*iter);
  }
  return true;
}


void SpiDevice::PrePortStop() {
  SpiPorts::iterator iter = m_spi_ports.begin();
  for (uint8_t i = 0; iter != m_spi_ports.end(); iter++, i++) {
    ostringstream str;
    m_preferences->SetValue(DeviceLabelKey(i), (*iter)->GetDeviceLabel());
    str << static_cast<int>((*iter)->GetPersonality());
    m_preferences->SetValue(PersonalityKey(i), str.str());
    str.str("");
    str << (*iter)->GetStartAddress();
    m_preferences->SetValue(StartAddressKey(i), str.str());
    str.str("");
    str << (*iter)->PixelCount();
    m_preferences->SetValue(PixelCountKey(i), str.str());
  }
  m_preferences->Save();
}

string SpiDevice::SpiBackendKey() const {
  return m_spi_device_name + "-backend";
}

string SpiDevice::SpiSpeedKey() const {
  return m_spi_device_name + "-spi-speed";
}

string SpiDevice::SpiCEKey() const {
  return m_spi_device_name + "-spi-ce-high";
}

string SpiDevice::PortCountKey() const {
  return m_spi_device_name + "-ports";
}

string SpiDevice::SyncPortKey() const {
  return m_spi_device_name + "-sync-port";
}

string SpiDevice::GPIOPinKey() const {
  return m_spi_device_name + "-gpio-pin";
}

string SpiDevice::DeviceLabelKey(uint8_t port) const {
  return GetPortKey("device-label", port);
}

string SpiDevice::PersonalityKey(uint8_t port) const {
  return GetPortKey("personality", port);
}

string SpiDevice::StartAddressKey(uint8_t port) const {
  return GetPortKey("dmx-address", port);
}

string SpiDevice::PixelCountKey(uint8_t port) const {
  return GetPortKey("pixel-count", port);
}

string SpiDevice::GetPortKey(const string &suffix, uint8_t port) const {
  std::ostringstream str;
  str << m_spi_device_name << "-" << static_cast<int>(port) << "-" << suffix;
  return str.str();
}

void SpiDevice::SetDefaults() {
  // Set device options
  set<string> valid_backends;
  valid_backends.insert(HARDWARE_BACKEND);
  valid_backends.insert(SOFTWARE_BACKEND);
  m_preferences->SetDefaultValue(SpiBackendKey(),
                                 SetValidator<string>(valid_backends),
                                 SOFTWARE_BACKEND);
  m_preferences->SetDefaultValue(SpiSpeedKey(), UIntValidator(0, 32000000),
                                 1000000);
  m_preferences->SetDefaultValue(SpiCEKey(), BoolValidator(), false);
  m_preferences->SetDefaultValue(PortCountKey(), UIntValidator(1, 8), 1);
  m_preferences->SetDefaultValue(SyncPortKey(), IntValidator(-2, 8), 0);
  m_preferences->Save();
}

void SpiDevice::PopulateHardwareBackendOptions(
    HardwareBackend::Options *options) {
  vector<string> pins = m_preferences->GetMultipleValue(GPIOPinKey());
  vector<string>::const_iterator iter = pins.begin();
  for (; iter != pins.end(); iter++) {
    uint16_t pin;
    if (!StringToInt(*iter, &pin)) {
      OLA_WARN << "Invalid GPIO pin " << *iter;
      continue;
    }

    if (pin > MAX_GPIO_PIN) {
      OLA_WARN << "Invalid GPIO pin " << *iter << ", must be < "
               << static_cast<int>(MAX_GPIO_PIN);
      continue;
    }

    options->gpio_pins.push_back(pin);
  }
}

void SpiDevice::PopulateSoftwareBackendOptions(
    SoftwareBackend::Options *options) {
  if (!StringToInt(m_preferences->GetValue(PortCountKey()),
                                           &options->outputs)) {
    OLA_WARN << "Invalid integer value for " << PortCountKey();
  }

  if (!StringToInt(m_preferences->GetValue(SyncPortKey()),
                                           &options->sync_output)) {
    OLA_WARN << "Invalid integer value for " << SyncPortKey();
  }
  if (options->sync_output == -2) {
    options->sync_output = options->outputs - 1;
  }
}

void SpiDevice::PopulateWriterOptions(SpiWriter::Options *options) {
  uint32_t spi_speed;
  if (StringToInt(m_preferences->GetValue(SpiSpeedKey()), &spi_speed)) {
    options->spi_speed = spi_speed;
  }
  bool ce_high;
  if (StringToBool(m_preferences->GetValue(SpiCEKey()), &ce_high)) {
    options->cs_enable_high = ce_high;
  }
}
}  // namespace spi
}  // namespace plugin
}  // namespace ola
