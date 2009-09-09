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
 * CID.cpp
 * CID class
 * Copyright (C) 2007 Simon Newton
 */

#include <string.h>
#include <string>
#include "CID.h"

namespace ola {
namespace e131 {

void CID::Pack(uint8_t *buf) const {
  memcpy(buf, m_uuid, CID_LENGTH);
}


CID& CID::operator=(const CID& other) {
  if (this != &other) {
    uuid_copy(m_uuid, other.m_uuid);
  }
  return *this;
}


bool CID::operator==(const CID& c1) const {
  return !uuid_compare(m_uuid, c1.m_uuid);
}


bool CID::operator!=(const CID& c1) const {
  return uuid_compare(m_uuid, c1.m_uuid);
}


std::string CID::ToString() const {
  char str[37];
  uuid_unparse(m_uuid, str);
  return std::string(str);
}


CID CID::Generate() {
  uuid_t uuid;
  uuid_generate(uuid);
  return CID(uuid);
}


CID CID::FromData(const uint8_t *data) {
  uuid_t uuid;
  uuid_copy(uuid, data);
  return CID(uuid);
}


CID CID::FromString(const std::string &cid) {
  uuid_t uuid;
  int ret = uuid_parse(cid.data(), uuid);
  if (ret == -1)
    uuid_clear(uuid);
  return CID(uuid);
}

} // e131
} // ola
