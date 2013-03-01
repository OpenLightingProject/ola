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

#include <string>
#include "olad/Device.h"
#include "ola/io/SelectServer.h"
#include "ola/rdm/UID.h"

namespace ola {
namespace plugin {
namespace spi {

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
    class Preferences *m_preferences;
    class PluginAdaptor *m_plugin_adaptor;
    class SPIOutputPort *m_port;
    string m_spi_device_name;

    string PersonalityKey() const;
    string StartAddressKey() const;
    string SPISpeedKey() const;
    string PixelCountKey() const;

    static const char SPI_DEVICE_NAME[];
};
}  // spi
}  // plugin
}  // ola
#endif  // PLUGINS_SPI_SPIDEVICE_H_
