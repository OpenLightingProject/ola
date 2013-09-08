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
 * SPIDevice.h
 * The SPI Device class
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_SPI_SPIDEVICE_H_
#define PLUGINS_SPI_SPIDEVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "ola/io/SelectServer.h"
#include "ola/rdm/UID.h"
#include "olad/Device.h"
#include "plugins/spi/SPIBackend.h"

namespace ola {
namespace plugin {
namespace spi {

using std::auto_ptr;

class SPIDevice: public ola::Device {
  public:
    SPIDevice(class SPIPlugin *owner,
              class Preferences *preferences,
              class PluginAdaptor *plugin_adaptor,
              const string &spi_device,
              const ola::rdm::UID &uid);

    string DeviceId() const;

  protected:
    bool StartHook();
    void PrePortStop();

  private:
    typedef std::vector<class SPIOutputPort*> SPIPorts;

    auto_ptr<SPIBackend> m_backend;
    class Preferences *m_preferences;
    class PluginAdaptor *m_plugin_adaptor;
    SPIPorts m_spi_ports;
    string m_spi_device_name;

    // Per device options
    string SPIBackendKey() const;
    string SPISpeedKey() const;
    string PortCountKey() const;

    // Per port options
    string PersonalityKey(uint8_t port) const;
    string PixelCountKey(uint8_t port) const;
    string StartAddressKey(uint8_t port) const;
    string GetPortKey(const string &suffix, uint8_t port) const;

    void SetDefaults();
    void PopulateHardwareBackendOptions(HardwareBackend::Options *options);
    void PopulateSoftwareBackendOptions(SoftwareBackend::Options *options);
    void PopulateOptions(SPIBackend::Options *options);

    static const char SPI_DEVICE_NAME[];
    static const char HARDWARE_BACKEND[];
    static const char SOFTWARE_BACKEND[];
};
}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIDEVICE_H_
