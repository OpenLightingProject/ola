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
 * DeviceManager.cpp
 * Implementation of the Device Manager, this object tracks what devices are in
 * use.
 * Copyright (C) 2005 Simon Newton
 */

#include "olad/plugin_api/DeviceManager.h"

#include <stdio.h>
#include <errno.h>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"
#include "olad/Port.h"
#include "olad/plugin_api/PortManager.h"

namespace ola {

using std::map;
using std::set;
using std::string;
using std::vector;

const unsigned int DeviceManager::MISSING_DEVICE_ALIAS = 0;
const char DeviceManager::PORT_PREFERENCES[] = "port";
const char DeviceManager::PRIORITY_VALUE_SUFFIX[] = "_priority_value";
const char DeviceManager::PRIORITY_MODE_SUFFIX[] = "_priority_mode";

bool operator <(const device_alias_pair& left,
                const device_alias_pair &right) {
  if (left.alias < right.alias)
    return true;
  return false;
}

DeviceManager::DeviceManager(PreferencesFactory *prefs_factory,
                             PortManager *port_manager)
    : m_port_preferences(NULL),
      m_port_manager(port_manager),
      m_next_device_alias(FIRST_DEVICE_ALIAS) {
  if (prefs_factory) {
    m_port_preferences = prefs_factory->NewPreference(PORT_PREFERENCES);
    m_port_preferences->Load();
  }
}

DeviceManager::~DeviceManager() {
  if (m_port_preferences) {
    m_port_preferences->Save();
  }
}

bool DeviceManager::RegisterDevice(AbstractDevice *device) {
  if (!device) {
    return false;
  }

  const string device_id = device->UniqueId();

  if (device_id.empty()) {
    OLA_WARN << "Device: " << device->Name() << " is missing UniqueId";
    return false;
  }

  // See if we already have an alias for this device.
  unsigned int alias;
  DeviceIdMap::iterator iter = m_devices.find(device_id);
  if (iter != m_devices.end()) {
    if (iter->second.device) {
      // already registered
      OLA_INFO << "Device " << device_id << " is already registered";
      return false;
    } else {
      // was previously registered, reuse alias
      alias = iter->second.alias;
      iter->second.device = device;
    }
  } else {
    alias = m_next_device_alias++;
    device_alias_pair pair(alias, device);
    STLReplace(&m_devices, device_id, pair);
  }

  STLReplace(&m_alias_map, alias, device);
  OLA_INFO << "Installed device: " << device->Name() << ":"
           << device->UniqueId();

  vector<InputPort*> input_ports;
  device->InputPorts(&input_ports);
  RestorePortSettings(input_ports);

  vector<OutputPort*> output_ports;
  device->OutputPorts(&output_ports);
  RestorePortSettings(output_ports);

  // look for timecode ports and add them to the set
  vector<OutputPort*>::const_iterator output_iter = output_ports.begin();
  for (; output_iter != output_ports.end(); ++output_iter) {
    if ((*output_iter)->SupportsTimeCode()) {
      m_timecode_ports.insert(*output_iter);
    }
  }

  return true;
}

bool DeviceManager::UnregisterDevice(const string &device_id) {
  device_alias_pair *pair = STLFind(&m_devices, device_id);
  if (!pair || !pair->device) {
    OLA_WARN << "Device " << device_id << "not found";
    return false;
  }

  ReleaseDevice(pair->device);
  STLRemove(&m_alias_map, pair->alias);

  pair->device = NULL;
  return true;
}

bool DeviceManager::UnregisterDevice(const AbstractDevice *device) {
  if (!device) {
    return false;
  }

  string device_id = device->UniqueId();
  if (device_id.empty()) {
    return false;
  }

  return UnregisterDevice(device_id);
}

unsigned int DeviceManager::DeviceCount() const {
  return m_alias_map.size();
}

vector<device_alias_pair> DeviceManager::Devices() const {
  vector<device_alias_pair> result;
  map<string, device_alias_pair>::const_iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    if (iter->second.device) {
      result.push_back(iter->second);
    }
  }
  return result;
}

AbstractDevice *DeviceManager::GetDevice(unsigned int alias) const {
  return STLFindOrNull(m_alias_map, alias);
}

device_alias_pair DeviceManager::GetDevice(const string &unique_id) const {
  const device_alias_pair *result = STLFind(&m_devices, unique_id);
  if (result) {
    return *result;
  } else {
    return device_alias_pair(MISSING_DEVICE_ALIAS, NULL);
  }
}

void DeviceManager::UnregisterAllDevices() {
  DeviceIdMap::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    ReleaseDevice(iter->second.device);
    iter->second.device = NULL;
  }
  m_alias_map.clear();
}

void DeviceManager::SendTimeCode(const ola::timecode::TimeCode &timecode) {
  set<OutputPort*>::iterator iter = m_timecode_ports.begin();
  for (; iter != m_timecode_ports.end(); iter++) {
    (*iter)->SendTimeCode(timecode);
  }
}

/*
 * Save the port universe patchings for a device
 * @param device the device to save the settings for
 */
void DeviceManager::ReleaseDevice(const AbstractDevice *device) {
  if (!m_port_preferences || !device) {
    return;
  }

  vector<InputPort*> input_ports;
  vector<OutputPort*> output_ports;
  device->InputPorts(&input_ports);
  device->OutputPorts(&output_ports);
  SavePortPatchings(input_ports);
  SavePortPatchings(output_ports);

  vector<InputPort*>::const_iterator input_iter = input_ports.begin();
  for (; input_iter != input_ports.end(); ++input_iter) {
    SavePortPriority(**input_iter);
  }

  vector<OutputPort*>::const_iterator output_iter = output_ports.begin();
  for (; output_iter != output_ports.end(); ++output_iter) {
    SavePortPriority(**output_iter);

    // remove from the timecode port set
    STLRemove(&m_timecode_ports, *output_iter);
  }
}


/*
 * Save the patching information for a list of ports.
 */
template <class PortClass>
void DeviceManager::SavePortPatchings(const vector<PortClass*> &ports) const {
  typename vector<PortClass*>::const_iterator iter = ports.begin();
  while (iter != ports.end()) {
    string port_id = (*iter)->UniqueId();
    if (port_id.empty()) {
      return;
    }

    if ((*iter)->GetUniverse()) {
      m_port_preferences->SetValue(
          port_id,
          IntToString((*iter)->GetUniverse()->UniverseId()));
    } else {
      m_port_preferences->RemoveValue(port_id);
    }
    iter++;
  }
}


/*
 * Save the priorities for all ports on this device
 */
void DeviceManager::SavePortPriority(const Port &port) const {
  if (port.PriorityCapability() == CAPABILITY_NONE) {
    return;
  }

  string port_id = port.UniqueId();
  if (port_id.empty()) {
    return;
  }

  m_port_preferences->SetValue(port_id + PRIORITY_VALUE_SUFFIX,
                               IntToString(port.GetPriority()));

  if (port.PriorityCapability() == CAPABILITY_FULL) {
    m_port_preferences->SetValue(port_id + PRIORITY_MODE_SUFFIX,
                                 IntToString(port.GetPriorityMode()));
  }
}


/*
 * Restore the priority settings for a port
 */
void DeviceManager::RestorePortPriority(Port *port) const {
  if (port->PriorityCapability() == CAPABILITY_NONE) {
    return;
  }

  string port_id = port->UniqueId();
  if (port_id.empty()) {
    return;
  }

  string priority_str = m_port_preferences->GetValue(
      port_id + PRIORITY_VALUE_SUFFIX);
  string priority_mode_str = m_port_preferences->GetValue(
      port_id + PRIORITY_MODE_SUFFIX);

  if (priority_str.empty() && priority_mode_str.empty()) {
    return;
  }

  uint8_t priority, priority_mode;
  // setting the priority to override mode first means we remember the over
  // value even if it's in inherit mode
  if (StringToInt(priority_str, &priority)) {
    m_port_manager->SetPriorityStatic(port, priority);
  }

  if (StringToInt(priority_mode_str, &priority_mode)) {
    if (priority_mode == PRIORITY_MODE_INHERIT) {
      m_port_manager->SetPriorityInherit(port);
    }
  }
}


/*
 * Restore the patching information for a port.
 */
template <class PortClass>
void DeviceManager::RestorePortSettings(const vector<PortClass*> &ports) const {
  if (!m_port_preferences) {
    return;
  }

  typename vector<PortClass*>::const_iterator iter = ports.begin();
  while (iter != ports.end()) {
    RestorePortPriority(*iter);
    PortClass *port = *iter;
    iter++;

    string port_id = port->UniqueId();
    if (port_id.empty())
      continue;

    string uni_id = m_port_preferences->GetValue(port_id);
    if (uni_id.empty())
      continue;

    errno = 0;
    int id = static_cast<int>(strtol(uni_id.c_str(), NULL, 10));
    if ((id == 0 && errno) || id < 0)
      continue;

    m_port_manager->PatchPort(port, id);
  }
}
}  // namespace ola
