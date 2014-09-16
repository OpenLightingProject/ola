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

    std::string DeviceId() const { return m_dev_path; }
    ola::io::ConnectedDescriptor *GetSocket() const;

 protected:
    bool StartHook();
    void PrePortStop();

 private:
    std::auto_ptr<class RenardWidget> m_widget;
    const std::string m_dev_path;
    class Preferences *m_preferences;

    // Per device options
    std::string DeviceBaudrateKey() const;
    std::string DeviceChannelsKey() const;
    std::string DeviceDmxOffsetKey() const;

    void SetDefaults();

    static const char RENARD_DEVICE_NAME[];
    static const uint8_t RENARD_START_ADDRESS;
    static const uint8_t RENARD_AVAILABLE_ADDRESSES;
    static const uint8_t DEFAULT_DMX_OFFSET;
    static const uint8_t DEFAULT_NUM_CHANNELS;
    static const uint32_t DEFAULT_BAUDRATE;
};
}  // namespace renard
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_RENARD_RENARDDEVICE_H_
