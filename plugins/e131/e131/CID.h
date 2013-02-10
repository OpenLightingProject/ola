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
 * Interface for the CID class
 * Copyright (C) 2007 Simon Newton
 */

#ifndef PLUGINS_E131_E131_CID_H_
#define PLUGINS_E131_E131_CID_H_

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdint.h>

#ifdef HAVE_OSSP_UUID_H
#include <ossp/uuid.h>
#else
#ifdef HAVE_UUID_UUID_H
#include <uuid/uuid.h>
#else
#include <uuid.h>
#endif
#endif

#include <iostream>
#include <string>

namespace ola {
namespace plugin {
namespace e131 {

class CID {
  public :
    enum { CID_LENGTH = 16 };

    CID();
    CID(const CID& other);
    ~CID();

    bool IsNil() const;
    void Pack(uint8_t *buf) const;
    std::string ToString() const;

    CID& operator=(const CID& c1);
    bool operator==(const CID& c1) const;
    bool operator!=(const CID& c1) const;

    static CID Generate();
    static CID FromData(const uint8_t *data);
    static CID FromString(const std::string &cid);

  private:
#ifdef USE_OSSP_UUID
    uuid_t *m_uuid;
    explicit CID(uuid_t *uuid);
#else
    uuid_t m_uuid;
    explicit CID(uuid_t uuid);
#endif
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_CID_H_
