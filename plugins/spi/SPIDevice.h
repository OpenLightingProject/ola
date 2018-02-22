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
 * SPIDevice.h
 * The SPI Device class
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_SPI_SPIDEVICE_H_
#define PLUGINS_SPI_SPIDEVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "olad/Device.h"
#include "ola/io/SelectServer.h"
#include "ola/rdm/UIDAllocator.h"
#include "ola/rdm/UID.h"
#include "plugins/spi/SPIBackend.h"
#include "plugins/spi/SPIWriter.h"

namespace ola {
namespace plugin {
namespace spi {


class SPIDevice: public ola::Device {
 public:
  SPIDevice(class SPIPlugin *owner,
            class Preferences *preferences,
            class PluginAdaptor *plugin_adaptor,
            const std::string &spi_device,
            ola::rdm::UIDAllocator *uid_allocator);

  std::string DeviceId() const;

  bool AllowMultiPortPatching() const { return true; }

 protected:
  bool StartHook();
  void PrePortStop();

 private:
  typedef std::vector<class SPIOutputPort*> SPIPorts;

  std::auto_ptr<SPIWriterInterface> m_writer;
  std::auto_ptr<SPIBackendInterface> m_backend;
  class Preferences *m_preferences;
  class PluginAdaptor *m_plugin_adaptor;
  SPIPorts m_spi_ports;
  std::string m_spi_device_name;

  // Per device options
  std::string SPIBackendKey() const;
  std::string SPISpeedKey() const;
  std::string SPICEKey() const;
  std::string PortCountKey() const;
  std::string SyncPortKey() const;
  std::string GPIOPinKey() const;

  // Per port options
  std::string DeviceLabelKey(uint8_t port) const;
  std::string PersonalityKey(uint8_t port) const;
  std::string PixelCountKey(uint8_t port) const;
  std::string StartAddressKey(uint8_t port) const;
  std::string GetPortKey(const std::string &suffix, uint8_t port) const;

  void SetDefaults();
  void PopulateHardwareBackendOptions(HardwareBackend::Options *options);
  void PopulateSoftwareBackendOptions(SoftwareBackend::Options *options);
  void PopulateWriterOptions(SPIWriter::Options *options);

  static const char SPI_DEVICE_NAME[];
  static const char HARDWARE_BACKEND[];
  static const char SOFTWARE_BACKEND[];
  static const uint16_t MAX_GPIO_PIN = 1023;
  static const uint32_t MAX_SPI_SPEED = 32000000;
  static const uint16_t MAX_PORT_COUNT = 32;
};
}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIDEVICE_H_
