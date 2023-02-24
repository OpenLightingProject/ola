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
 * RootInflator.cpp
 * The Inflator for the root level packets over UDP
 * Copyright (C) 2007 Simon Newton
 */

#include "ola/Logging.h"
#include "libs/acn/RootInflator.h"

namespace ola {
namespace acn {

using ola::acn::CID;

/*
 * Decode the root headers. If data is null we're expected to use the last
 * header we got.
 * @param headers the HeaderSet to add to
 * @param data a pointer to the data
 * @param length length of the data
 * @returns true if successful, false otherwise
 */
bool RootInflator::DecodeHeader(HeaderSet *headers,
                                const uint8_t *data,
                                unsigned int length,
                                unsigned int *bytes_used) {
  if (data) {
    if (length >= CID::CID_LENGTH) {
      CID cid = CID::FromData(data);
      m_last_hdr.SetCid(cid);
      headers->SetRootHeader(m_last_hdr);
      *bytes_used = CID::CID_LENGTH;
      return true;
    }
    return false;
  }
  *bytes_used = 0;
  if (m_last_hdr.GetCid().IsNil()) {
    OLA_WARN << "Missing CID data";
    return false;
  }
  headers->SetRootHeader(m_last_hdr);
  return true;
}


/*
 * Reset the header field
 */
void RootInflator::ResetHeaderField() {
  CID cid;
  m_last_hdr.SetCid(cid);
}


/**
 * This runs the on_data callback if we have one
 */
bool RootInflator::PostHeader(uint32_t, const HeaderSet &headers) {
  if (m_on_data.get())
    m_on_data->Run(headers.GetTransportHeader());
  return true;
}
}  // namespace acn
}  // namespace ola
