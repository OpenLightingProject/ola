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
 * E131Port.cpp
 * The E1.31 plugin for ola
 * Copyright (C) 2007-2009 Simon Newton
 */

#include <string.h>

#include <ola/Logging.h>
#include <olad/Universe.h>
#include "E131Port.h"
#include "E131Device.h"


namespace ola {
namespace e131 {

bool E131Port::IsOutput() const {
  // odd ports are output
  return (PortId() % 2);
}


bool E131Port::WriteDMX(const DmxBuffer &buffer) {
  E131Device *device = GetDevice();

  if (!IsOutput())
    return false;

  Universe *universe = GetUniverse();
  if (!universe)
    return false;

  E131Node *node = device->GetNode();
  return node->SendDMX(universe->UniverseId(), buffer);
}


const DmxBuffer &E131Port::ReadDMX() const {
  return m_buffer;
}


/*
 * Override this so we can setup the callback, we also want to make sure only
 * one port can be patched to a universe at one time.
 */
bool E131Port::SetUniverse(Universe *universe) {

  // check for valid ranges here
  if (universe && universe->UniverseId() == MAX_TWO_BYTE) {
    OLA_WARN << "Universe id " << universe->UniverseId() << "> " <<
      MAX_TWO_BYTE;
    return false;
  }

  E131Device *device = GetDevice();
  E131Node *node = device->GetNode();
  Universe *old_universe = GetUniverse();

  if (!universe) {
    // unpatch
    if (IsOutput())
      node->SetSourceName(old_universe->UniverseId(), "");
    else if (old_universe)
      node->RemoveHandler(old_universe->UniverseId());

  } else {
    // patch request, check no other ports are using this universe
    vector<AbstractPort*> ports = device->Ports();
    vector<AbstractPort*>::const_iterator iter;
    for (iter = ports.begin(); iter != ports.end(); ++iter) {
      if ((*iter)->IsOutput() == IsOutput() &&
          (*iter)->GetUniverse() &&
          (*(*iter)->GetUniverse()) == *universe) {

        OLA_WARN << "Port " << (*iter)->PortId() <<
          " is already patched to universe " << universe->UniverseId();
        return false;
      }
    }

    if (IsOutput()) {
      node->SetSourceName(universe->UniverseId(), universe->Name());
    } else {
      // setup callback
      node->SetHandler(universe->UniverseId(), &m_buffer,
                       NewClosure<E131Port>(this, &E131Port::DmxChanged));
    }
  }

  Port<E131Device>::SetUniverse(universe);
  return true;
}


string E131Port::Description() const {
  std::stringstream str;
  if (GetUniverse())
    str << "E.131 Universe " << GetUniverse()->UniverseId();
  return str.str();
}


/*
 * Update the universe name
 */
void E131Port::UniverseNameChanged(const string &new_name) {
  E131Device *device = GetDevice();
  E131Node *node = device->GetNode();
  node->SetSourceName(GetUniverse()->UniverseId(), new_name);
}


} //plugin
} //ola
