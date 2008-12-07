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
 * Implementation of the device manager, this object tracks what devices are in use
 * Copyright (C) 2005 Simon Newton
 */

#include <stdio.h>
#include <llad/logger.h>
#include "DeviceManager.h"

namespace lla {

/*
 * Register a device
 *
 * @param device pointer to the device to register
 * @return 0 on sucess, -1 on failure
 */
int DeviceManager::RegisterDevice(AbstractDevice *device) {
  device->SetDeviceId(m_next_device_id);
  m_devices.push_back(device);
  m_next_device_id++;
  Logger::instance()->log(Logger::INFO, "Installed device");
  return 0;
}


/*
 * Unregister this device
 *
 * @param dev pointer to the AbstractDevice to unregister
 * @return 0 on sucess, non 0 on failure
 */
int DeviceManager::UnregisterDevice(AbstractDevice *device) {
  vector<AbstractDevice*>::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    if ((*iter)->DeviceId() == device->DeviceId()) {
      m_devices.erase(iter);
      break;
    }
  }
  return 0;
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
  m_devices.clear();
  m_next_device_id = 1;
}

} //lla
