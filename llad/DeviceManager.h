/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * DeviceManager.h
 * Interface to the DeviceManager class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef LLA_DEVICEMANAGER_H
#define LLA_DEVICEMANAGER_H

#include <vector>
#include <llad/Device.h>
#include "UniverseStore.h"
#include <llad/Preferences.h>

namespace lla {

using std::vector;

class DeviceManager {
  public:
    DeviceManager(PreferencesFactory *prefs_factory,
                  UniverseStore *universe_store);
    ~DeviceManager();

    bool RegisterDevice(AbstractDevice *device);
    bool UnregisterDevice(AbstractDevice *device);
    vector<AbstractDevice*> Devices() const { return m_devices; }
    unsigned int DeviceCount() const { return m_devices.size(); }
    AbstractDevice* GetDevice(unsigned int device_id);

    void UnregisterAllDevices();

  private:
    UniverseStore *m_universe_store;
    Preferences *m_port_preferences;
    vector<AbstractDevice*> m_devices;    // list of devices
    unsigned int m_next_device_id;

    DeviceManager(const DeviceManager&);
    DeviceManager& operator=(const DeviceManager&);
    void SaveDevicePortSettings(AbstractDevice *device);

    static const string PORT_PREFERENCES;
};


} //lla
#endif
