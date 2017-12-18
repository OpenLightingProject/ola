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
 */

#ifndef OLAD_PLUGIN_API_DEVICEMANAGER_H_
#define OLAD_PLUGIN_API_DEVICEMANAGER_H_

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
class device_alias_pair {
 public:
  unsigned int alias;
  AbstractDevice *device;

  device_alias_pair(unsigned int alias, AbstractDevice *device)
      : alias(alias), device(device) {}
};

bool operator <(const device_alias_pair& left, const device_alias_pair &right);

/**
 * @brief Keeps track of OLA's devices.
 *
 * Devices can be identified in one of two ways, by device-id or by alias.
 * device-ids are strings and are persistent across restarting olad and reload
 * the plugins. device-ids are the keys used in preference containers to
 * identify devices.
 *
 * Device aliases are unsigned integers and are only valid for the lifetime of
 * the DeviceManager object. Device aliases are used by users when patching /
 * controlling a device. since '1' is easier to understand / type
 * than 5-02050016. If a device is registered, then unregistered, then
 * registered again, it'll have the same device alias.
 */
class DeviceManager {
 public:
  /**
   * @brief Create a new DeviceManager.
   * @param prefs_factory the PreferencesFactory to use, ownership is not
   *   transferred.
   * @param port_manager the PortManager to use, ownership is not transferred.
   */
  DeviceManager(PreferencesFactory *prefs_factory,
                class PortManager *port_manager);

  /**
   * @brief Destructor.
   */
  ~DeviceManager();

  /**
   * @brief Register a device.
   * @param device The device to register, ownership is not transferred.
   * @returns true on success, false on failure.
   *
   * During registration, any saved port patchings for this device are restored.
   */
  bool RegisterDevice(AbstractDevice *device);

  /**
   * @brief Unregister a device by id.
   * @param device_id the id of the device to remove
   * @returns true on success, false on failure
   */
  bool UnregisterDevice(const std::string &device_id);

  /**
   * @brief Unregister a device by pointer.
   * @param device a pointer to the device.
   * @returns true on success, false on failure
   */
  bool UnregisterDevice(const AbstractDevice *device);

  /**
   * @brief Return the number of devices.
   * @returns the number of devices.
   */
  unsigned int DeviceCount() const;

  /**
   * @brief Return a list of all devices and their aliases.
   * @returns a vector of device_alias_pairs.
   */
  std::vector<device_alias_pair> Devices() const;

  /**
   * @brief Lookup a device using the device alias.
   * @returns a pointer to the device or NULL if the device wasn't found.
   */
  AbstractDevice *GetDevice(unsigned int alias) const;

  /**
   * @brief Lookup a device using the device id.
   * @param unique_id the device id of the device.
   * @returns a device_alias_pair, if the device isn't found the alias is set to
   *   MISSING_DEVICE_ALIAS and the device pointer is NULL.
   */
  device_alias_pair GetDevice(const std::string &unique_id) const;

  /**
   * @brief Remove all devices.
   */
  void UnregisterAllDevices();


  /**
   * @brief Send timecode to all ports which support timecode.
   * @param timecode the TimeCode information.
   */
  void SendTimeCode(const ola::timecode::TimeCode &timecode);

  static const unsigned int MISSING_DEVICE_ALIAS;

 private:
  typedef std::map<std::string, device_alias_pair> DeviceIdMap;
  typedef std::map<unsigned int, AbstractDevice*> DeviceAliasMap;

  Preferences *m_port_preferences;
  class PortManager *m_port_manager;

  DeviceIdMap m_devices;
  DeviceAliasMap m_alias_map;

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
  static const char PRIORITY_VALUE_SUFFIX[];
  static const char PRIORITY_MODE_SUFFIX[];

  DISALLOW_COPY_AND_ASSIGN(DeviceManager);
};
}  // namespace ola
#endif  // OLAD_PLUGIN_API_DEVICEMANAGER_H_
