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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * CIDImpl.cpp
 * The actual implementation of a CID. The implementation changes based on
 *   which uuid library is installed.
 * Copyright (C) 2007 Simon Newton
 */

#include <string.h>
#include <string>
#include "libs/acn/CIDImpl.h"

namespace ola {
namespace acn {

using std::string;

#ifdef USE_OSSP_UUID
CIDImpl::CIDImpl()
  : m_uuid(NULL) {
}

CIDImpl::CIDImpl(uuid_t *uuid)
  : m_uuid(uuid) {
}

CIDImpl::CIDImpl(const CIDImpl& other)
  : m_uuid(NULL) {
  if (other.m_uuid)
    uuid_clone(other.m_uuid, &m_uuid);
}

CIDImpl::~CIDImpl() {
  if (m_uuid)
    uuid_destroy(m_uuid);
}

bool CIDImpl::IsNil() const {
  if (!m_uuid)
    return true;

  int result;
  uuid_isnil(m_uuid, &result);
  return result;
}

/**
 * Pack a CIDImpl into the binary representation
 */
void CIDImpl::Pack(uint8_t *buffer) const {
  size_t data_length = CIDImpl_LENGTH;
  // buffer may not be 4 byte aligned
  char uid_data[CIDImpl_LENGTH];
  void *ptr = static_cast<void*>(uid_data);
  if (m_uuid) {
    uuid_export(m_uuid, UUID_FMT_BIN, &ptr, &data_length);
    memcpy(buffer, uid_data, CIDImpl_LENGTH);
  } else {
    memset(buffer, 0, CIDImpl_LENGTH);
  }
}

CIDImpl& CIDImpl::operator=(const CIDImpl& other) {
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

bool CIDImpl::operator==(const CIDImpl& c1) const {
  int result;
  uuid_compare(m_uuid, c1.m_uuid, &result);
  return 0 == result;
}

bool CIDImpl::operator!=(const CIDImpl& c1) const {
  return !(*this == c1);
}

bool CIDImpl::operator<(const CIDImpl& c1) const {
  int result;
  uuid_compare(m_uuid, c1.m_uuid, &result);
  return result < 0;
}

string CIDImpl::ToString() const {
  char cid[UUID_LEN_STR + 1];
  void *str = static_cast<void*>(cid);
  size_t length = UUID_LEN_STR + 1;
  uuid_export(m_uuid, UUID_FMT_STR, &str, &length);
  return string(cid);
}

void CIDImpl::Write(ola::io::OutputBufferInterface *output) const {
  size_t data_length = CIDImpl_LENGTH;
  // buffer may not be 4 byte aligned
  uint8_t uid_data[CIDImpl_LENGTH];
  void *ptr = static_cast<void*>(uid_data);
  if (m_uuid) {
    uuid_export(m_uuid, UUID_FMT_BIN, &ptr, &data_length);
  } else {
    memset(ptr, 0, CIDImpl_LENGTH);
  }
  output->Write(uid_data, CIDImpl_LENGTH);
}


CIDImpl* CIDImpl::Generate() {
  uuid_t *uuid;
  uuid_create(&uuid);
  uuid_make(uuid, UUID_MAKE_V4);
  return new CIDImpl(uuid);
}


CIDImpl* CIDImpl::FromData(const uint8_t *data) {
  uuid_t *uuid;
  uuid_create(&uuid);
  uuid_import(uuid, UUID_FMT_BIN, data, CIDImpl_LENGTH);
  return new CIDImpl(uuid);
}


CIDImpl* CIDImpl::FromString(const string &cid) {
  uuid_t *uuid;
  uuid_create(&uuid);
  uuid_import(uuid, UUID_FMT_STR, cid.data(), cid.length());
  return new CIDImpl(uuid);
}


#else
// We're using the e2fs utils uuid library

CIDImpl::CIDImpl() {
  uuid_clear(m_uuid);
}

CIDImpl::CIDImpl(uuid_t uuid) {
  uuid_copy(m_uuid, uuid);
}

CIDImpl::CIDImpl(const CIDImpl& other) {
  uuid_copy(m_uuid, other.m_uuid);
}

CIDImpl::~CIDImpl() {}

bool CIDImpl::IsNil() const {
  return uuid_is_null(m_uuid);
}

void CIDImpl::Pack(uint8_t *buf) const {
  memcpy(buf, m_uuid, CIDImpl_LENGTH);
}

void CIDImpl::Write(ola::io::OutputBufferInterface *output) const {
  output->Write(m_uuid, CIDImpl_LENGTH);
}

CIDImpl& CIDImpl::operator=(const CIDImpl& other) {
  if (this != &other) {
    uuid_copy(m_uuid, other.m_uuid);
  }
  return *this;
}

bool CIDImpl::operator==(const CIDImpl& c1) const {
  return !uuid_compare(m_uuid, c1.m_uuid);
}

bool CIDImpl::operator!=(const CIDImpl& c1) const {
  return uuid_compare(m_uuid, c1.m_uuid);
}

bool CIDImpl::operator<(const CIDImpl& c1) const {
  return uuid_compare(m_uuid, c1.m_uuid) < 0;
}

string CIDImpl::ToString() const {
  char str[37];
  uuid_unparse(m_uuid, str);
  return string(str);
}


CIDImpl* CIDImpl::Generate() {
  uuid_t uuid;
  uuid_generate(uuid);
  return new CIDImpl(uuid);
}

CIDImpl* CIDImpl::FromData(const uint8_t *data) {
  uuid_t uuid;
  uuid_copy(uuid, data);
  return new CIDImpl(uuid);
}

CIDImpl* CIDImpl::FromString(const string &cid) {
  uuid_t uuid;
  int ret = uuid_parse(cid.data(), uuid);
  if (ret == -1)
    uuid_clear(uuid);
  return new CIDImpl(uuid);
}
#endif  // end the e2fs progs uuid implementation
}  // namespace acn
}  // namespace ola
