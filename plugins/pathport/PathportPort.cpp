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
 *
 * SandNetPort.cpp
 * The SandNet plugin for ola
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <sstream>
#include <string>
#include "ola/Logging.h"
#include "ola/BaseTypes.h"

#include "plugins/pathport/PathportPort.h"

namespace ola {
namespace plugin {
namespace pathport {


string PathportPortHelper::Description(const Universe *universe) const {
  if (!universe)
    return "";

  std::stringstream str;
  str << "Pathport xDMX " << DMX_UNIVERSE_SIZE * universe->UniverseId() <<
    " - " << DMX_UNIVERSE_SIZE * (1 + universe->UniverseId()) - 1;
  return str.str();
}


/*
 * Don't allow us to patch ports out of range
 */
bool PathportPortHelper::PreSetUniverse(Universe *new_universe) {
  if (new_universe &&
      new_universe->UniverseId() > PathportNode::MAX_UNIVERSES) {
      OLA_WARN << "Pathport universes need to be between 0 and " <<
          PathportNode::MAX_UNIVERSES;
      return false;
  }
  return true;
}


/*
 * Write operation
 */
bool PathportOutputPort::WriteDMX(const DmxBuffer &buffer) {
  if (GetUniverse())
    return m_node->SendDMX(GetUniverse()->UniverseId(), buffer);
}
}  // pathport
}  // plugin
}  // ola
