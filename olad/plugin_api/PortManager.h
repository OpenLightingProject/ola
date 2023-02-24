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
 * PortManager.h
 * Provides a unified interface for controlling port patchings & priorities.
 * Copyright (C) 2005 Simon Newton
 */

#ifndef OLAD_PLUGIN_API_PORTMANAGER_H_
#define OLAD_PLUGIN_API_PORTMANAGER_H_

#include <vector>
#include "olad/Device.h"
#include "olad/PortBroker.h"
#include "olad/plugin_api/DeviceManager.h"
#include "olad/plugin_api/UniverseStore.h"
#include "ola/base/Macro.h"


namespace ola {

/**
 * @brief Responsible for performing Port operations.
 */
class PortManager {
 public:
  /**
   * @brief Create a new PortManager.
   * @param universe_store the UniverseStore used to lookup / create Universes.
   * @param broker the PortBroker to update when Ports are added / removed.
   */
  PortManager(UniverseStore *universe_store,
              PortBroker *broker)
      : m_universe_store(universe_store),
        m_broker(broker) {
  }

  /**
   * @brief Destructor.
   */
  ~PortManager() {}

  /**
   * @brief Patch an InputPort to a universe.
   * @param port the port to patch
   * @param universe the universe-id to patch to.
   * @returns true is successful, false otherwise
   */
  bool PatchPort(InputPort *port, unsigned int universe);

  /**
   * @brief Patch an OutputPort to a universe.
   * @param port the port to patch
   * @param universe the universe-id to patch to.
   * @returns true is successful, false otherwise
   */
  bool PatchPort(OutputPort *port, unsigned int universe);

  /**
   * @brief UnPatch an InputPort.
   * @param port the port to unpatch
   * @returns true is successful, false otherwise
   */
  bool UnPatchPort(InputPort *port);

  /**
   * @brief UnPatch an OutputPort.
   * @param port the port to unpatch
   * @returns true is successful, false otherwise
   */
  bool UnPatchPort(OutputPort *port);

  /**
   * @brief Set a port to 'inherit' priority mode.
   * @param port the port to configure
   */
  bool SetPriorityInherit(Port *port);

  /**
   * @brief Set a port to 'override' priority mode.
   * @param port the port to configure
   * @param value the new priority
   */
  bool SetPriorityStatic(Port *port, uint8_t value);

 private:
  template<class PortClass>
  bool GenericPatchPort(PortClass *port,
                        unsigned int new_universe_id);

  template<class PortClass>
  bool GenericUnPatchPort(PortClass *port);

  template<class PortClass>
  bool CheckLooping(const AbstractDevice *device,
                    unsigned int new_universe_id) const;

  template<class PortClass>
  bool CheckMultiPort(const AbstractDevice *device,
                      unsigned int new_universe_id) const;

  /**
   * Check if any input ports in this device are bound to the universe.
   * @returns true if there is a match, false otherwise.
   */
  bool CheckInputPortsForUniverse(const AbstractDevice *device,
                                  unsigned int universe_id) const;

  /**
   * Check if any output ports in this device are bound to the universe.
   * @returns true if there is a match, false otherwise.
   */
  bool CheckOutputPortsForUniverse(const AbstractDevice *device,
                                   unsigned int universe_id) const;

  /**
   * Check for any port in a list that's bound to this universe.
   * @returns true if there is a match, false otherwise.
   */
  template<class PortClass>
  bool CheckForPortMatchingUniverse(const std::vector<PortClass*> &ports,
                                    unsigned int universe_id) const;

  UniverseStore * const m_universe_store;
  PortBroker *m_broker;

  DISALLOW_COPY_AND_ASSIGN(PortManager);
};
}  // namespace ola
#endif  // OLAD_PLUGIN_API_PORTMANAGER_H_
