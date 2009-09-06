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

#include <olad/Universe.h>
#include "E131Port.h"
#include "E131Device.h"


namespace ola {
namespace e131 {

bool E131Port::CanRead() const {
  // even ports are input
  return !(PortId() % 2);
}


bool E131Port::CanWrite() const {
  // odd ports are output
  return (PortId() % 2);
}


string E131Port::Description() const {
  std::stringstream str;
  str << "E131 " << PortId();
  return str.str();
}


bool E131Port::WriteDMX(const DmxBuffer &buffer) {
  E131Device *device = GetDevice();

  if (!CanWrite())
    return false;

  E131Node *node = device->GetNode();
  if (!node->SendDMX(PortId(), buffer))
    return false;
  return true;
}

const DmxBuffer &E131Port::ReadDMX() const {
  return m_buffer;
}

/*
 * override this so we can set the callback
int E131Port::set_universe(Universe *uni) {

  if (get_universe())
    m_layer->unregister_uni(get_universe()->get_uid());

  Port::set_universe(uni);

  if (can_read() && uni)
   m_layer->register_uni(uni->get_uid(), data_callback, (void*) this);
  return 0;
}
*/

} //plugin
} //ola
