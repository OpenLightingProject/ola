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
 * PortManager.cpp
 * Enables the Patching of Ports
 * Copyright (C) 2005 Simon Newton
 */

#include "olad/plugin_api/PortManager.h"

#include <vector>
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "olad/Port.h"

namespace ola {

using std::vector;

bool PortManager::PatchPort(InputPort *port,
                            unsigned int universe) {
  return GenericPatchPort(port, universe);
}

bool PortManager::PatchPort(OutputPort *port,
                            unsigned int universe) {
  return GenericPatchPort(port, universe);
}

bool PortManager::UnPatchPort(InputPort *port) {
  return GenericUnPatchPort(port);
}

bool PortManager::UnPatchPort(OutputPort *port) {
  return GenericUnPatchPort(port);
}

bool PortManager::SetPriorityInherit(Port *port) {
  if (port->PriorityCapability() != CAPABILITY_FULL)
    return true;

  if (port->GetPriorityMode() != PRIORITY_MODE_INHERIT) {
    port->SetPriorityMode(PRIORITY_MODE_INHERIT);
  }

  return true;
}

bool PortManager::SetPriorityStatic(Port *port, uint8_t value) {
  if (port->PriorityCapability() == CAPABILITY_NONE)
    return true;

  if (port->PriorityCapability() == CAPABILITY_FULL &&
      port->GetPriorityMode() != PRIORITY_MODE_STATIC)
    port->SetPriorityMode(PRIORITY_MODE_STATIC);

  if (value > ola::dmx::SOURCE_PRIORITY_MAX) {
    OLA_WARN << "Priority " << static_cast<int>(value)
        << " is greater than the max priority ("
        << static_cast<int>(ola::dmx::SOURCE_PRIORITY_MAX) << ")";
    value = ola::dmx::SOURCE_PRIORITY_MAX;
  }

  if (port->GetPriority() != value)
    port->SetPriority(value);
  return true;
}


template<class PortClass>
bool PortManager::GenericPatchPort(PortClass *port,
                                   unsigned int new_universe_id) {
  if (!port)
    return false;

  Universe *universe = port->GetUniverse();
  if (universe && universe->UniverseId() == new_universe_id)
    return true;

  AbstractDevice *device = port->GetDevice();
  if (device) {
    if (!device->AllowLooping()) {
      // check ports of the opposite type
      if (CheckLooping<PortClass>(device, new_universe_id))
        return false;
    }

    if (!device->AllowMultiPortPatching()) {
      if (CheckMultiPort<PortClass>(device, new_universe_id))
        return false;
    }
  }

  // unpatch if required
  if (universe) {
    OLA_DEBUG << "Port " << port->UniqueId() << " is bound to universe " <<
      universe->UniverseId();
    m_broker->RemovePort(port);
    universe->RemovePort(port);
  }

  universe = m_universe_store->GetUniverseOrCreate(new_universe_id);
  if (!universe)
    return false;

  if (port->SetUniverse(universe)) {
    OLA_INFO << "Patched " << port->UniqueId() << " to universe " <<
      universe->UniverseId();
    m_broker->AddPort(port);
    universe->AddPort(port);
  } else {
    if (!universe->IsActive())
      m_universe_store->AddUniverseGarbageCollection(universe);
  }
  return true;
}


template<class PortClass>
bool PortManager::GenericUnPatchPort(PortClass *port) {
  if (!port)
    return false;

  Universe *universe = port->GetUniverse();
  m_broker->RemovePort(port);
  if (universe) {
    universe->RemovePort(port);
    port->SetUniverse(NULL);
    OLA_INFO << "Unpatched " << port->UniqueId() << " from uni "
      << universe->UniverseId();
  }
  return true;
}


template <class PortClass>
bool PortManager::CheckLooping(const AbstractDevice *device,
                               unsigned int new_universe_id) const {
  return CheckOutputPortsForUniverse(device, new_universe_id);
}

template <>
bool PortManager::CheckLooping<OutputPort>(
    const AbstractDevice *device,
    unsigned int new_universe_id) const {
  return CheckInputPortsForUniverse(device, new_universe_id);
}


template <class PortClass>
bool PortManager::CheckMultiPort(const AbstractDevice *device,
                                 unsigned int new_universe_id) const {
  return CheckInputPortsForUniverse(device, new_universe_id);
}


template <>
bool PortManager::CheckMultiPort<OutputPort>(
    const AbstractDevice *device,
    unsigned int new_universe_id) const {
  return CheckOutputPortsForUniverse(device, new_universe_id);
}

bool PortManager::CheckInputPortsForUniverse(const AbstractDevice *device,
                                             unsigned int universe_id) const {
  vector<InputPort*> ports;
  device->InputPorts(&ports);
  return CheckForPortMatchingUniverse(ports, universe_id);
}

bool PortManager::CheckOutputPortsForUniverse(const AbstractDevice *device,
                                              unsigned int universe_id) const {
  vector<OutputPort*> ports;
  device->OutputPorts(&ports);
  return CheckForPortMatchingUniverse(ports, universe_id);
}

template<class PortClass>
bool PortManager::CheckForPortMatchingUniverse(
    const vector<PortClass*> &ports,
    unsigned int universe_id) const {
  typename vector<PortClass*>::const_iterator iter;
  for (iter = ports.begin(); iter != ports.end(); ++iter) {
    if ((*iter)->GetUniverse() &&
        (*iter)->GetUniverse()->UniverseId() == universe_id) {
      OLA_INFO << "Port " << (*iter)->PortId() << " is already patched to "
        << universe_id;
      return true;
    }
  }
  return false;
}
}  // namespace ola
