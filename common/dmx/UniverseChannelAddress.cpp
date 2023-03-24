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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * UniverseChannelAddress.cpp
 * Represents a universe-channel address pair.
 * Copyright (C) 2023 Peter Newman
 */

#include <assert.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/dmx/UniverseChannelAddress.h>
#include <string.h>
#include <string>

namespace ola {
namespace dmx {

using std::string;


string UniverseChannelAddress::ToString() const {
  std::ostringstream str;
  str << Universe() << ":" << Channel();
  return str.str();
}


/**
 * Extract a UniverseChannelAddress from a string.
 */
bool UniverseChannelAddress::FromString(
    const string &input,
    UniverseChannelAddress *universe_channel_address) {
  size_t pos = input.find_first_of(":");
  if (pos == string::npos) {
    return false;
  }

  unsigned int universe;
  if (!StringToInt(input.substr(0, pos), &universe)) {
    return false;
  }
  uint16_t channel;
  if (!StringToInt(input.substr(pos + 1), &channel)) {
    return false;
  }
  *universe_channel_address = UniverseChannelAddress(universe, channel);
  return true;
}


UniverseChannelAddress UniverseChannelAddress::FromStringOrDie(
    const string &address) {
  UniverseChannelAddress universe_channel_address;
  assert(FromString(address, &universe_channel_address));
  return universe_channel_address;
}
}  // namespace dmx
}  // namespace ola
