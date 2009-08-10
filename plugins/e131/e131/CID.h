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
 * CID.h
 * Interface for the CID class
 * Copyright (C) 2007 Simon Newton
 */

#ifndef OLA_E131_CID_H
#define OLA_E131_CID_H

#include <stdint.h>
#include <string>
#include <uuid/uuid.h>


#include <iostream>

namespace ola {
namespace e131 {

class CID {
  public :
    enum { CID_LENGTH = 16 };

    CID() { uuid_clear(m_uuid); }
    CID(uuid_t uuid) { uuid_copy(m_uuid, uuid); }
    CID(const CID& other) { uuid_copy(m_uuid, other.m_uuid); }

    bool IsNil() const { return uuid_is_null(m_uuid); }
    void Pack(uint8_t *buf) const;
    std::string ToString() const;

    CID& operator=(const CID& c1);
    bool operator==(const CID& c1) const;
    bool operator!=(const CID& c1) const;

    static CID Generate();
    static CID FromData(const uint8_t *data);

  private:
    uuid_t m_uuid;
};

} // e131
} // ola
#endif
