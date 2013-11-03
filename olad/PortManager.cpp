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
 * PortManager.cpp
 * Enables the Patching of Ports
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string>
#include <vector>
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "olad/PortManager.h"
#include "olad/Port.h"

namespace ola {

/*
 * Patch a port
 * @param port the port to patch
 * @param universe the universe to patch to
 * @returns true is successful, false otherwise
 */
bool PortManager::PatchPort(InputPort *port,
                            unsigned int universe) {
  return GenericPatchPort(port, universe);
}


/*
 * Patch a port
 * @param port the port to patch
 * @param universe the universe to patch to
 * @returns true is successful, false otherwise
 */
bool PortManager::PatchPort(OutputPort *port,
                            unsigned int universe) {
  return GenericPatchPort(port, universe);
}


/*
 * UnPatch a port
 * @param port the port to unpatch
 * @returns true is successful, false otherwise
 */
bool PortManager::UnPatchPort(InputPort *port) {
  return GenericUnPatchPort(port);
}


/*
 * UnPatch a port
 * @param port the port to unpatch
 * @returns true is successful, false otherwise
 */
bool PortManager::UnPatchPort(OutputPort *port) {
  return GenericUnPatchPort(port);
}


/*
 * Set a port to inherit priority mode.
 * @param port the port to configure
 */
bool PortManager::SetPriorityInherit(Port *port) {
  if (port->PriorityCapability() == CAPABILITY_NONE)
    return true;

  if (port->GetPriorityMode() != PRIORITY_MODE_INHERIT)
    port->SetPriorityMode(PRIORITY_MODE_INHERIT);
  return true;
}


/*
 * Set a port to override priority mode.
 * @param port the port to configure
 * @param value the new priority
 */
bool PortManager::SetPriorityOverride(Port *port, uint8_t value) {
  if (port->PriorityCapability() == CAPABILITY_NONE)
    return true;

  if (port->PriorityCapability() == CAPABILITY_FULL &&
      port->GetPriorityMode() != PRIORITY_MODE_OVERRIDE)
    port->SetPriorityMode(PRIORITY_MODE_OVERRIDE);

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
    OLA_DEBUG << "Port " << port->UniqueId() << " has been removed from uni "
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


/*
 * Check if any input ports in this device are bound to the universe.
 * @returns true if there is a match, false otherwise.
 */
bool PortManager::CheckInputPortsForUniverse(const AbstractDevice *device,
                                             unsigned int universe_id) const {
  vector<InputPort*> ports;
  device->InputPorts(&ports);
  return CheckForPortMatchingUniverse(ports, universe_id);
}


/*
 * Check if any output ports in this device are bound to the universe.
 * @returns true if there is a match, false otherwise.
 */
bool PortManager::CheckOutputPortsForUniverse(const AbstractDevice *device,
                                              unsigned int universe_id) const {
  vector<OutputPort*> ports;
  device->OutputPorts(&ports);
  return CheckForPortMatchingUniverse(ports, universe_id);
}


/*
 * Check for any port in a list that's bound to this universe.
 * @returns true if there is a match, false otherwise.
 */
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
