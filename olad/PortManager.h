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
 * PortManager.h
 * Provides a unified interface for controlling port patchings & priorities.
 * Copyright (C) 2005-2010 Simon Newton
 */

#ifndef OLAD_PORTMANAGER_H_
#define OLAD_PORTMANAGER_H_

#include <string>
#include <vector>
#include "olad/Device.h"
#include "olad/DeviceManager.h"
#include "olad/UniverseStore.h"


namespace ola {

class PortManager {
  public:
    explicit PortManager(UniverseStore *universe_store)
        : m_universe_store(universe_store) {
    }
    ~PortManager() {}

    bool PatchPort(InputPort *port, unsigned int universe);
    bool PatchPort(OutputPort *port, unsigned int universe);
    bool UnPatchPort(InputPort *port);
    bool UnPatchPort(OutputPort *port);

    bool SetPriorityInherit(Port *port);
    bool SetPriorityOverride(Port *port, uint8_t value);

  private:
    PortManager(const PortManager&);
    PortManager& operator=(const PortManager&);

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
    bool CheckForPortMatchingUniverse(const vector<PortClass*> &ports,
                                      unsigned int universe_id) const;

    UniverseStore * const m_universe_store;
};
}  // ola
#endif  // OLAD_PORTMANAGER_H_
