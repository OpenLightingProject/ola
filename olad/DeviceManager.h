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
 * DeviceManager.h
 * Interface to the DeviceManager class
 * Copyright (C) 2005 Simon Newton
 *
 * The DeviceManager assigns an unsigned int as an alias to each device which
 * remains consistent throughout the lifetime of the DeviceManager. These are
 * used in the user facing portion as '1' is easier to understand/type
 * than 5-02050016. If a device is registered, then unregistered, then
 * registered again, it'll have the same device alias.
 *
 * The DeviceManager is also responsible for restoring the port patchings when
 * devices are registered.
 */

#ifndef OLAD_DEVICEMANAGER_H_
#define OLAD_DEVICEMANAGER_H_

#include <map>
#include <set>
#include <string>
#include <vector>
#include "ola/base/Macro.h"
#include "ola/timecode/TimeCode.h"
#include "olad/Device.h"
#include "olad/Preferences.h"

namespace ola {

// pair a device with it's alias
typedef struct {
  unsigned int alias;
  AbstractDevice *device;
} device_alias_pair;

bool operator <(const device_alias_pair& left, const device_alias_pair &right);

class DeviceManager {
 public:
    DeviceManager(PreferencesFactory *prefs_factory,
                  class PortManager *port_manager);
    ~DeviceManager();

    bool RegisterDevice(AbstractDevice *device);
    bool UnregisterDevice(const std::string &device_id);
    bool UnregisterDevice(const AbstractDevice *device);
    unsigned int DeviceCount() const;
    std::vector<device_alias_pair> Devices() const;
    AbstractDevice *GetDevice(unsigned int alias) const;
    device_alias_pair GetDevice(const std::string &unique_id) const;
    void UnregisterAllDevices();

    void SendTimeCode(const ola::timecode::TimeCode &timecode);

    static const unsigned int MISSING_DEVICE_ALIAS;
    static const char PRIORITY_VALUE_SUFFIX[];
    static const char PRIORITY_MODE_SUFFIX[];

 private:
    Preferences *m_port_preferences;
    class PortManager *m_port_manager;
    // map device_ids to devices
    std::map<std::string, device_alias_pair> m_devices;
    // map alias to devices
    std::map<unsigned int, AbstractDevice*> m_alias_map;
    unsigned int m_next_device_alias;
    std::set<class OutputPort*> m_timecode_ports;

    void ReleaseDevice(const AbstractDevice *device);
    void RestoreDevicePortSettings(AbstractDevice *device);

    template <class PortClass>
    void SavePortPatchings(const std::vector<PortClass*> &ports) const;

    void SavePortPriority(const Port &port) const;
    void RestorePortPriority(Port *port) const;

    template <class PortClass>
    void RestorePortSettings(const std::vector<PortClass*> &ports) const;

    static const char PORT_PREFERENCES[];
    static const unsigned int FIRST_DEVICE_ALIAS = 1;

    DISALLOW_COPY_AND_ASSIGN(DeviceManager);
};
}  // namespace ola
#endif  // OLAD_DEVICEMANAGER_H_
