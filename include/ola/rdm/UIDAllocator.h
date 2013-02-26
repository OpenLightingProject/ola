/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * UIDAllocator.h
 * A class to allocate UIDs
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_UIDALLOCATOR_H_
#define INCLUDE_OLA_RDM_UIDALLOCATOR_H_

#include <ola/rdm/UID.h>

namespace ola {
namespace rdm {

/**
 * Given a starting UID, this returns successive UIDs until the space is
 * exhausted.
 */
class UIDAllocator {
  public:
    explicit UIDAllocator(const UID &uid)
      : m_esta_id(uid.ManufacturerId()),
        m_device_id(uid.DeviceId()),
        m_last_device_id(UID::ALL_DEVICES) {
    }

    UIDAllocator(const UID &uid, uint32_t last_device_id)
      : m_esta_id(uid.ManufacturerId()),
        m_device_id(uid.DeviceId()),
        m_last_device_id(last_device_id) {
    }

    // Returns the next UID in the range or NULL if not available UIDs remain.
    UID *AllocateNext() {
      if (m_device_id == m_last_device_id)
        return NULL;

      UID *uid = new UID(m_esta_id, m_device_id);
      m_device_id++;
      return uid;
    }

  private:
    uint16_t m_esta_id;
    uint32_t m_device_id;
    uint32_t m_last_device_id;
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_UIDALLOCATOR_H_
