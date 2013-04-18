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
 * CID.h
 * The CID class, this just wraps a CIDImpl so we don't need to include all the
 *   UID headers.
 * Copyright (C) 2007 Simon Newton
 */

#ifndef INCLUDE_OLA_ACN_CID_H_
#define INCLUDE_OLA_ACN_CID_H_

#include <stdint.h>
#include <ola/io/OutputBuffer.h>
#include <string>

namespace ola {
namespace acn {

class CID {
  public :
    enum { CID_LENGTH = 16 };

    CID();
    CID(const CID& other);
    ~CID();

    bool IsNil() const;
    void Pack(uint8_t *buf) const;
    std::string ToString() const;
    void Write(ola::io::OutputBufferInterface *output) const;

    CID& operator=(const CID& c1);
    bool operator==(const CID& c1) const;
    bool operator!=(const CID& c1) const;

    static CID Generate();
    static CID FromData(const uint8_t *data);
    static CID FromString(const std::string &cid);

  private:
    class CIDImpl *m_impl;

    // Takes ownership;
    explicit CID(class CIDImpl *impl);
};
}  // acn
}  // ola
#endif  // INCLUDE_OLA_ACN_CID_H_
