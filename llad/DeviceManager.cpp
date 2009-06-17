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
 * devicemanager.cpp
 * Implementation of the device manager, this object tracks what devices are in
 * use.
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <stdio.h>
#include <errno.h>
#include <lla/Logging.h>
#include <lla/StringUtils.h>
#include <llad/Port.h>
#include "DeviceManager.h"

namespace lla {

const string DeviceManager::PORT_PREFERENCES = "port";

/*
 * Constructor
 */
DeviceManager::DeviceManager(PreferencesFactory *prefs_factory,
                             UniverseStore *universe_store):
    m_universe_store(universe_store),
    m_port_preferences(NULL),
    m_next_device_id(1) {

  if (prefs_factory) {
    m_port_preferences = prefs_factory->NewPreference(PORT_PREFERENCES);
    m_port_preferences->Load();
  }
}

/*
 * Cleanup
 */
DeviceManager::~DeviceManager() {
  if (m_port_preferences)
    m_port_preferences->Save();
}


/*
 * Register a device
 * @param device pointer to the device to register
 * @return true on sucess, false on failure
 */
bool DeviceManager::RegisterDevice(AbstractDevice *device) {
  device->SetDeviceId(m_next_device_id);
  m_devices.push_back(device);
  m_next_device_id++;
  LLA_INFO << "Installed device: " << device->Name();

  if (!m_port_preferences)
    return true;

  vector<AbstractPort*> ports = device->Ports();
  for (vector<AbstractPort*>::iterator port_iter = ports.begin();
       port_iter != ports.end(); ++port_iter) {
    if ((*port_iter)->UniqueId().empty())
      continue;

    string uni_id = m_port_preferences->GetValue((*port_iter)->UniqueId());
    if (uni_id.empty())
      continue;

    errno = 0;
    int id = (int) strtol(uni_id.data(), NULL, 10);
    if (id == 0 && errno)
      continue;
    LLA_INFO << "Restored dev " << device->DeviceId() << ", port " <<
      (*port_iter)->PortId() << " to universe " << id;
    Universe *uni = m_universe_store->GetUniverseOrCreate(id);
    (*port_iter)->SetUniverse(uni);
  }
  return true;
}


/*
 * Unregister this device
 * @param dev pointer to the AbstractDevice to unregister
 * @return true on sucess, false on failure
 */
bool DeviceManager::UnregisterDevice(AbstractDevice *device) {
  vector<AbstractDevice*>::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    if ((*iter)->DeviceId() == device->DeviceId()) {
      SaveDevicePortSettings(device);
      m_devices.erase(iter);
      break;
    }
  }
  return true;
}


/*
 * Return the device with the id of device_id
 */
AbstractDevice* DeviceManager::GetDevice(unsigned int device_id) {
  vector<AbstractDevice*>::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    if ((*iter)->DeviceId() == device_id) {
      return *iter;
    }
  }
  return NULL;
}


/*
 * Remove all devices and reset the device counter
 */
void DeviceManager::UnregisterAllDevices() {
  vector<AbstractDevice*>::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter)
    SaveDevicePortSettings(*iter);

  m_devices.clear();
  m_next_device_id = 1;
}


/*
 * Save the port settings for a device
 */
void DeviceManager::SaveDevicePortSettings(AbstractDevice *device) {
  if (!m_port_preferences)
    return;

  vector<AbstractPort*> ports = device->Ports();
  vector<AbstractPort*>::iterator port_iter;
  for (port_iter = ports.begin(); port_iter != ports.end(); ++port_iter) {
    if ((*port_iter)->UniqueId().empty())
      continue;

    if (!(*port_iter)->GetUniverse())
      continue;

    m_port_preferences->SetValue(
        (*port_iter)->UniqueId(),
        IntToString((*port_iter)->GetUniverse()->UniverseId()));
  }
}

} //lla
