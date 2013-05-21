/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * UID.cpp
 * The UID class.
 * Copyright (C) 2011 Simon Newton
 */

#include <string>
#include <vector>
#include "ola/StringUtils.h"
#include "ola/rdm/UID.h"

namespace ola {
namespace rdm {

UID* UID::FromString(const string &uid) {
  std::vector<string> tokens;
  ola::StringSplit(uid, tokens, ":");

  if (tokens.size() != 2 || tokens[0].size() != 4 || tokens[1].size() != 8)
    return NULL;

  uint16_t esta_id;
  unsigned int device_id;
  if (!ola::HexStringToInt(tokens[0], &esta_id))
    return NULL;
  if (!ola::HexStringToInt(tokens[1], &device_id))
    return NULL;

  return new UID(esta_id, device_id);
}
}  // namespace rdm
}  // namespace ola
