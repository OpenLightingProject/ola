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
 * RenardDevice.h
 * Interface for the renard device
 * Copyright (C) 2013 Hakan Lindestaf
 */

#ifndef PLUGINS_RENARD_RENARDDEVICE_H_
#define PLUGINS_RENARD_RENARDDEVICE_H_

#include <memory>
#include <string>

#include "olad/Device.h"

namespace ola {

class AbstractPlugin;

namespace plugin {
namespace renard {

class RenardDevice: public ola::Device {
  public:
    RenardDevice(AbstractPlugin *owner,
                 class Preferences *preferences,
                 const std::string &dev_path);
    ~RenardDevice();

    std::string DeviceId() const { return m_device_name; }
    ola::io::ConnectedDescriptor *GetSocket() const;

  protected:
    bool StartHook();
    void PrePortStop();

  private:
    std::string m_path;
    std::auto_ptr<class RenardWidget> m_widget;
    std::string m_device_name;
    class Preferences *m_preferences;

    // Per device options
    string DeviceBaudrateKey() const;
    string DeviceChannelsKey() const;
    string DeviceDmxOffsetKey() const;

    void SetDefaults();

    static const char RENARD_DEVICE_NAME[];
    static const uint8_t RENARD_CHANNELS_IN_BANK;
    static const uint8_t RENARD_AVAILABLE_ADDRESSES;
    static const uint8_t DEFAULT_DMX_OFFSET;
    static const uint8_t DEFAULT_NUM_CHANNELS;
    static const uint32_t DEFAULT_BAUDRATE;
    static const char BAUDRATE_19200[];
    static const char BAUDRATE_38400[];
    static const char BAUDRATE_57600[];
    static const char BAUDRATE_115200[];
};
}  // namespace renard
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_RENARD_RENARDDEVICE_H_
