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

#ifndef OLAD_PORTMANAGER_H_
#define OLAD_PORTMANAGER_H_

#include <vector>
#include "olad/Device.h"
#include "olad/DeviceManager.h"
#include "olad/PortBroker.h"
#include "olad/UniverseStore.h"
#include "ola/base/Macro.h"


namespace ola {

class PortManager {
 public:
    explicit PortManager(UniverseStore *universe_store,
                         PortBroker *broker)
        : m_universe_store(universe_store),
          m_broker(broker) {
    }
    ~PortManager() {}

    bool PatchPort(InputPort *port, unsigned int universe);
    bool PatchPort(OutputPort *port, unsigned int universe);
    bool UnPatchPort(InputPort *port);
    bool UnPatchPort(OutputPort *port);

    bool SetPriorityInherit(Port *port);
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

    bool CheckInputPortsForUniverse(const AbstractDevice *device,
                                    unsigned int universe_id) const;
    bool CheckOutputPortsForUniverse(const AbstractDevice *device,
                                     unsigned int universe_id) const;

    template<class PortClass>
    bool CheckForPortMatchingUniverse(const std::vector<PortClass*> &ports,
                                      unsigned int universe_id) const;

    UniverseStore * const m_universe_store;
    PortBroker *m_broker;

    DISALLOW_COPY_AND_ASSIGN(PortManager);
};
}  // namespace ola
#endif  // OLAD_PORTMANAGER_H_
