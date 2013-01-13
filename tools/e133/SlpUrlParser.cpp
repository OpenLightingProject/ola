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
 * SlpUrlParser.h
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <string>
#include <vector>
#include "tools/e133/SlpConstants.h"
#include "tools/e133/SlpUrlParser.h"


using std::string;


/**
 * Extract the IP Address and UID from a E1.33 SLP URL.
 * @param url the SLP url
 * @param uid a pointer to a UID which is populated
 * @param ip a pointer to a IPV4Address which is populated
 * @returns true if this was a valid url, false otherise
 *
 * The url is expected to be in the form
 * service:rdmnet-device://192.168.1.204:5568/7a7000000001
 */
bool ParseSlpUrl(const string &url,
                 ola::rdm::UID *uid,
                 ola::network::IPV4Address *ip) {
  size_t url_size = url.length();
  size_t service_size = sizeof(E133_DEVICE_SLP_SERVICE_NAME);

  if (url_size < service_size + 1 ||
      url.compare(0, service_size - 1, E133_DEVICE_SLP_SERVICE_NAME))
    return false;

  if (url[service_size - 1] != ':')
    return false;

  std::vector<string> url_parts;
  ola::StringSplit(url, url_parts, ":");
  if (url_parts.size() != 4)
    return false;

  if (url_parts[2].compare(0, 2, "//"))
    return false;

  if (!ola::network::IPV4Address::FromString(url_parts[2].substr(2), ip))
    return false;

  std::vector<string> port_url_parts;
  ola::StringSplit(url_parts[3], port_url_parts, "/");
  if (port_url_parts.size() != 2)
    return false;

  if (port_url_parts[0] != string(ACN_PORT_STRING)) {
    return false;
  }

  const string &uid_str = port_url_parts[1];
  if (uid_str.size() != 2 * ola::rdm::UID::UID_SIZE)
    return false;

  uint16_t esta_id;
  unsigned int device_id;
  if (!ola::HexStringToInt(uid_str.substr(0, 4), &esta_id))
    return false;
  if (!ola::HexStringToInt(uid_str.substr(4, 8), &device_id))
    return false;

  ola::rdm::UID temp_uid(esta_id, device_id);
  *uid = temp_uid;
  return true;
}
