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
 * PortPatcher.h
 * Enables the Patching of Ports
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef OLAD_PORTPATCHER_H_
#define OLAD_PORTPATCHER_H_

#include <string>
#include <vector>
#include "olad/Device.h"
#include "olad/DeviceManager.h"
#include "olad/PortManager.h"
#include "olad/UniverseStore.h"


namespace ola {

class PortPatcher {
  public:
    explicit PortPatcher(UniverseStore *universe_store)
        : m_universe_store(universe_store) {
    }
    ~PortPatcher() {}

    bool PatchPort(InputPort *port, unsigned int universe);
    bool PatchPort(OutputPort *port, unsigned int universe);
    bool UnPatchPort(InputPort *port);
    bool UnPatchPort(OutputPort *port);

  private:
    PortPatcher(const PortPatcher&);
    PortPatcher& operator=(const PortPatcher&);

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
#endif  // OLAD_PORTPATCHER_H_
