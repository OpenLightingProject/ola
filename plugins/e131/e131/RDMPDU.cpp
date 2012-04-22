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
 * RDMPDU.cpp
 * The RDMPDU
 * Copyright (C) 2012 Simon Newton
 */


#include <string.h>
#include <ola/Logging.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/RDMCommand.h>
#include "plugins/e131/e131/RDMPDU.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::HostToNetwork;

/*
 * Size of the data portion
 */
unsigned int RDMPDU::DataSize() const {
  if (m_command)
    return 1 + m_command->Size();  // include the start code
  return 0;
}


/*
 * RDM PDUs don't contain a header.
 */
bool RDMPDU::PackHeader(uint8_t *, unsigned int &length) const {
  length = 0;
  return true;
}


/*
 * Pack the data portion.
 */
bool RDMPDU::PackData(uint8_t *data, unsigned int &length) const {
  if (!m_command) {
    length = 0;
    return true;
  }

  unsigned int size = length;
  bool r = m_command->Pack(data, &size);
  length = size;
  return r;
}
}  // ola
}  // e131
}  // plugin
