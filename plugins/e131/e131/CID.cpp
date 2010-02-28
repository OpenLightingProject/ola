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

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <string.h>
#include <string>
#include "plugins/e131/e131/CID.h"

namespace ola {
namespace plugin {
namespace e131 {

#ifdef USE_OSSP_UUID
CID::CID()
  : m_uuid(NULL) {
}


CID::CID(uuid_t *uuid)
  : m_uuid(uuid) {
}


CID::CID(const CID& other)
  : m_uuid(NULL) {
  if (other.m_uuid)
    uuid_clone(other.m_uuid, &m_uuid);
}

CID::~CID() {
  if (m_uuid)
    uuid_destroy(m_uuid);
}


bool CID::IsNil() const {
  if (!m_uuid)
    return true;

  int result;
  uuid_isnil(m_uuid, &result);
  return result;
}


void CID::Pack(uint8_t *buffer) const {
  size_t data_length = CID_LENGTH;
  char *ptr = reinterpret_cast<char*>(buffer);
  if (m_uuid)
    uuid_export(m_uuid, UUID_FMT_BIN, &ptr, &data_length);
  else
    memset(buffer, 0, CID_LENGTH);
}


CID& CID::operator=(const CID& other) {
  if (this != &other) {
    if (m_uuid)
      uuid_destroy(m_uuid);

    if (other.m_uuid)
      uuid_clone(other.m_uuid, &m_uuid);
    else
      m_uuid = NULL;
  }
  return *this;
}


bool CID::operator==(const CID& c1) const {
  int result;
  uuid_compare(m_uuid, c1.m_uuid, &result);
  return 0 == result;
}


bool CID::operator!=(const CID& c1) const {
  return !(*this == c1);
}


std::string CID::ToString() const {
  char cid[UUID_LEN_STR + 1];
  char *str = cid;
  size_t length = UUID_LEN_STR + 1;
  int r = uuid_export(m_uuid, UUID_FMT_STR, &str, &length);
  return std::string(str);
}


CID CID::Generate() {
  uuid_t *uuid;
  uuid_create(&uuid);
  uuid_make(uuid, UUID_MAKE_V4);
  return CID(uuid);
}


CID CID::FromData(const uint8_t *data) {
  uuid_t *uuid;
  uuid_create(&uuid);
  uuid_import(uuid, UUID_FMT_BIN, data, CID_LENGTH);
  return CID(uuid);
}


CID CID::FromString(const std::string &cid) {
  uuid_t *uuid;
  uuid_create(&uuid);
  uuid_import(uuid, UUID_FMT_STR, cid.data(), cid.length());
  return CID(uuid);
}


#else
// We're using the e2fs utils uuid library

CID::CID() {
  uuid_clear(m_uuid);
}


CID::CID(uuid_t uuid) {
  uuid_copy(m_uuid, uuid);
}


CID::CID(const CID& other) {
  uuid_copy(m_uuid, other.m_uuid);
}


CID::~CID() {
}


bool CID::IsNil() const {
  return uuid_is_null(m_uuid);
}


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
#endif  // end the e2fs progs uuid implementation
}  // e131
}  // plugin
}  // ola
