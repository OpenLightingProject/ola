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
 * CID.cpp
 * CID class, passes everything through to CIDImpl.
 * Copyright (C) 2007 Simon Newton
 */

#include <ola/acn/CID.h>
#include <string>
#include "libs/acn/CIDImpl.h"

namespace ola {
namespace acn {

using std::string;

CID::CID() : m_impl(new CIDImpl()) {}

CID::CID(const CID& other) : m_impl(new CIDImpl(*other.m_impl)) {}

CID::CID(CIDImpl *impl) : m_impl(impl) {}

CID::~CID() {
  delete m_impl;
}

bool CID::IsNil() const {
  return m_impl->IsNil();
}

void CID::Pack(uint8_t *buffer) const {
  return m_impl->Pack(buffer);
}

CID& CID::operator=(const CID& other) {
  *m_impl = *other.m_impl;
  return *this;
}

bool CID::operator==(const CID& other) const {
  return (*m_impl == *other.m_impl);
}

bool CID::operator!=(const CID& c1) const {
  return !(*this == c1);
}

bool CID::operator<(const CID& c1) const {
  return *m_impl < *c1.m_impl;
}

string CID::ToString() const {
  return m_impl->ToString();
}

void CID::Write(ola::io::OutputBufferInterface *output) const {
  m_impl->Write(output);
}


CID CID::Generate() {
  return CID(CIDImpl::Generate());
}


CID CID::FromData(const uint8_t *data) {
  return CID(CIDImpl::FromData(data));
}


CID CID::FromString(const string &cid) {
  return CID(CIDImpl::FromString(cid));
}
}  // namespace acn
}  // namespace ola
