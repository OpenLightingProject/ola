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
 * PortPatcher.cpp
 * Enables the Patching of Ports
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string>
#include <vector>
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "olad/PortPatcher.h"
#include "olad/Port.h"

namespace ola {

/*
 * Patch a port
 * @param port the port to patch
 * @param universe the universe to patch to
 * @returns true is successful, false otherwise
 */
bool PortPatcher::PatchPort(InputPort *port,
                            unsigned int universe) {
  return GenericPatchPort(port, universe);
}


/*
 * Patch a port
 * @param port the port to patch
 * @param universe the universe to patch to
 * @returns true is successful, false otherwise
 */
bool PortPatcher::PatchPort(OutputPort *port,
                            unsigned int universe) {
  return GenericPatchPort(port, universe);
}


/*
 * UnPatch a port
 * @param port the port to unpatch
 * @returns true is successful, false otherwise
 */
bool PortPatcher::UnPatchPort(InputPort *port) {
  return GenericUnPatchPort(port);
}


/*
 * UnPatch a port
 * @param port the port to unpatch
 * @returns true is successful, false otherwise
 */
bool PortPatcher::UnPatchPort(OutputPort *port) {
  return GenericUnPatchPort(port);
}


/*
 * Set the priority settings for a port. This only applies the settings if all
 * parameters are valid.
 * @param port the port to configure
 * @param mode the new mode
 * @param priority the new priority
 * @param pedantic don't take any action if there are errors, if this is false
 * we do the best we can and try to set a priority.
 */
bool PortPatcher::SetPriority(Port *port,
                              const string &mode_str,
                              const string &priority_str,
                              bool pedantic) {
  unsigned int mode = PRIORITY_MODE_INHERIT;
  unsigned int priority = Port::PORT_PRIORITY_DEFAULT;

  if (port->PriorityCapability() == CAPABILITY_FULL) {
    if (!StringToUInt(mode_str, &mode)) {
      OLA_WARN << "Invalid priority mode: " << mode_str;
      if (pedantic)
        return false;
    }
  }

  if (!StringToUInt(priority_str, &priority)) {
    OLA_WARN << "Invalid priority value: " << priority_str;
    if (pedantic)
      return false;
  }

  return SetPriority(port, mode, priority, pedantic);
}


/*
 * Set the priority settings for a port. This only applies the settings if all
 * parameters are valid.
 * @param port the port to configure
 * @param mode the new mode
 * @param priority the new priority
 * @param pedantic don't take any action if there are errors, if this is false
 * we do the best we can and try to set a priority.
 */
bool PortPatcher::SetPriority(Port *port,
                              unsigned int mode,
                              unsigned int priority,
                              bool pedantic) {
  if (port->PriorityCapability() == CAPABILITY_NONE)
    return true;

  if (priority > Port::PORT_PRIORITY_MAX) {
    OLA_WARN << "Priority " << priority <<
      " is greater than the max priority (" << Port::PORT_PRIORITY_MAX << ")";
    if (pedantic)
      return false;
    priority = Port::PORT_PRIORITY_MAX;
  }

  if (port->PriorityCapability() == CAPABILITY_FULL &&
      port->GetPriorityMode() != mode) {
    if (mode >= PRIORITY_MODE_END) {
      OLA_WARN << "Priority mode " << mode << " is out of range";
      if (pedantic)
        return false;
    } else {
      port->SetPriorityMode((port_priority_mode) mode);
    }
  }

  if (priority != port->GetPriority())
    port->SetPriority(priority);
  return true;
}


template<class PortClass>
bool PortPatcher::GenericPatchPort(PortClass *port,
                                   unsigned int new_universe_id) {
  if (!port)
    return false;

  Universe *universe = port->GetUniverse();
  if (universe && universe->UniverseId() == new_universe_id)
    return true;

  AbstractDevice *device = port->GetDevice();
  if (!device->AllowLooping()) {
    // check ports of the opposite type
    if (CheckLooping<PortClass>(device, new_universe_id))
      return false;
  }

  if (!device->AllowMultiPortPatching()) {
    if (CheckMultiPort<PortClass>(device, new_universe_id))
      return false;
  }

  // unpatch if required
  if (universe) {
    OLA_DEBUG << "Port " << port->UniqueId() << " is bound to universe " <<
      universe->UniverseId();
    universe->RemovePort(port);
  }

  universe = m_universe_store->GetUniverseOrCreate(new_universe_id);
  if (!universe)
    return false;

  OLA_INFO << "Patched " << port->UniqueId() << " to universe " <<
    universe->UniverseId();
  universe->AddPort(port);
  port->SetUniverse(universe);
  return true;
}


template<class PortClass>
bool PortPatcher::GenericUnPatchPort(PortClass *port) {
  if (!port)
    return false;

  Universe *universe = port->GetUniverse();
  if (universe) {
    universe->RemovePort(port);
    port->SetUniverse(NULL);
    OLA_DEBUG << "Port " << port->UniqueId() << " has been removed from uni "
      << universe->UniverseId();
  }
  return true;
}


template <class PortClass>
bool PortPatcher::CheckLooping(const AbstractDevice *device,
                               unsigned int new_universe_id) const {
  return CheckOutputPortsForUniverse(device, new_universe_id);
}

template <>
bool PortPatcher::CheckLooping<OutputPort>(
    const AbstractDevice *device,
    unsigned int new_universe_id) const {
  return CheckInputPortsForUniverse(device, new_universe_id);
}


template <class PortClass>
bool PortPatcher::CheckMultiPort(const AbstractDevice *device,
                                 unsigned int new_universe_id) const {
  return CheckInputPortsForUniverse(device, new_universe_id);
}


template <>
bool PortPatcher::CheckMultiPort<OutputPort>(
    const AbstractDevice *device,
    unsigned int new_universe_id) const {
  return CheckOutputPortsForUniverse(device, new_universe_id);
}


/*
 * Check if any input ports in this device are bound to the universe.
 * @returns true if there is a match, false otherwise.
 */
bool PortPatcher::CheckInputPortsForUniverse(const AbstractDevice *device,
                                             unsigned int universe_id) const {
  vector<InputPort*> ports;
  device->InputPorts(&ports);
  return CheckForPortMatchingUniverse(ports, universe_id);
}


/*
 * Check if any output ports in this device are bound to the universe.
 * @returns true if there is a match, false otherwise.
 */
bool PortPatcher::CheckOutputPortsForUniverse(const AbstractDevice *device,
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
bool PortPatcher::CheckForPortMatchingUniverse(
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
}  // ola
