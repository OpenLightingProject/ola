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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * E133URLParser.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/e133/E133URLParser.h>
#include <string>
#include <vector>
#include "tools/e133/SLPConstants.h"

namespace ola {
namespace e133 {

using ola::network::IPV4Address;
using ola::rdm::UID;
using std::string;

/**
 * Extract the IP Address and UID from a E1.33 SLP URL.
 * @param url the SLP url
 * @param uid a pointer to a UID which is populated
 * @param ip a pointer to a IPV4Address which is populated
 * @returns true if this was a valid url, false otherise
 *
 * The url is expected to be in the form
 * service:rdmnet-device://192.168.1.204/7a7000000001
 */
bool ParseE133URL(const string &url,
                  ola::rdm::UID *uid,
                  IPV4Address *ip) {
  size_t url_size = url.length();
  string prefix(E133_DEVICE_SLP_SERVICE_NAME);
  prefix.append("://");
  size_t prefix_size = prefix.length();

  if (url_size < prefix_size || url.compare(0, prefix_size, prefix))
    return false;

  const string remainder = url.substr(prefix_size);

  std::vector<string> url_parts;
  ola::StringSplit(remainder, url_parts, "/");
  if (url_parts.size() != 2)
    return false;

  if (!IPV4Address::FromString(url_parts[0], ip))
    return false;

  const string &uid_str = url_parts[1];
  if (uid_str.size() != 2 * UID::UID_SIZE)
    return false;

  uint16_t esta_id;
  unsigned int device_id;
  if (!ola::HexStringToInt(uid_str.substr(0, 4), &esta_id))
    return false;
  if (!ola::HexStringToInt(uid_str.substr(4, 8), &device_id))
    return false;

  UID temp_uid(esta_id, device_id);
  *uid = temp_uid;
  return true;
}
}  // e133
}  // ola
