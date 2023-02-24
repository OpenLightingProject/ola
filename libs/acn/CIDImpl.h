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
 * CIDImpl.h
 * The actual implementation of a CID. The implementation changes based on
 *   which uuid library is installed.
 * Copyright (C) 2007 Simon Newton
 */

#ifndef LIBS_ACN_CIDIMPL_H_
#define LIBS_ACN_CIDIMPL_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <stdint.h>

#ifdef HAVE_OSSP_UUID_H
#include <ossp/uuid.h>
#else
#ifdef HAVE_UUID_UUID_H
#include <uuid/uuid.h>
#else
#include <uuid.h>
#endif  // HAVE_UUID_UUID_H
#endif  // HAVE_OSSP_UUID_H

#include <iostream>
#include <string>

#include "ola/io/OutputBuffer.h"

namespace ola {
namespace acn {

class CIDImpl {
 public :
  enum { CIDImpl_LENGTH = 16 };

  CIDImpl();
  CIDImpl(const CIDImpl& other);
  ~CIDImpl();

  bool IsNil() const;
  void Pack(uint8_t *buf) const;
  std::string ToString() const;
  void Write(ola::io::OutputBufferInterface *output) const;

  CIDImpl& operator=(const CIDImpl& c1);
  bool operator==(const CIDImpl& c1) const;
  bool operator!=(const CIDImpl& c1) const;
  bool operator<(const CIDImpl& c1) const;

  static CIDImpl* Generate();
  static CIDImpl* FromData(const uint8_t *data);
  static CIDImpl* FromString(const std::string &cid);

 private:
#ifdef USE_OSSP_UUID
  uuid_t *m_uuid;
  explicit CIDImpl(uuid_t *uuid);
#else
  uuid_t m_uuid;
  explicit CIDImpl(uuid_t uuid);
#endif  // HAVE_OSSP_UUID
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_CIDIMPL_H_
