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
 * UID.h
 * Representation of an RDM UID
 * Copyright (C) 2005-2010 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_UID_H_
#define INCLUDE_OLA_RDM_UID_H_

#include <stdint.h>
#include <iomanip>
#include <string>

namespace ola {
namespace rdm {

using std::ostream;


/*
 * Represents a RDM UID.
 */
class UID {
  public:
    UID(uint16_t esta_id, uint32_t device_id) {
      m_uid.esta_id = esta_id;
      m_uid.device_id = device_id;
    }

    UID& operator=(const UID& other) {
      if (this != &other) {
        m_uid.esta_id = other.m_uid.esta_id;
        m_uid.device_id = other.m_uid.device_id;
      }
      return *this;
    }

    explicit UID(const uint8_t *data) {
      m_uid.esta_id = (data[0] << 8) + data[1];
      m_uid.device_id = ((data[2] << 24) + (data[3] << 16) + (data[4] << 8) +
          data[5]);
    }

    bool operator==(const UID &other) const {
      return 0 == cmp(*this, other);
    }

    bool operator!=(const UID &other) const {
      return 0 != cmp(*this, other);
    }

    bool operator>(const UID &other) const {
      return cmp(*this, other) > 0;
    }

    bool operator<(const UID &other) const {
      return cmp(*this, other) < 0;
    }

    uint16_t ManufacturerId() const { return m_uid.esta_id; }

    uint16_t DeviceId() const { return m_uid.device_id; }

    std::string ToString() const {
      std::stringstream str;
      str << std::setfill('0') << std::setw(4) << std::hex << m_uid.esta_id
        << ":" << std::setw(8) << m_uid.device_id;
      return str.str();
    }

    friend ostream& operator<< (ostream &out, const UID &uid) {
      return out << uid.ToString();
    }

    bool Pack(uint8_t *buffer, unsigned int length) const {
      if (length < UID_SIZE)
        return false;
      buffer[0] = m_uid.esta_id >> 8;
      buffer[1] = m_uid.esta_id & 0xff;
      buffer[2] = m_uid.device_id >> 24;
      buffer[3] = m_uid.device_id >> 16;
      buffer[4] = m_uid.device_id >> 8;
      buffer[5] = m_uid.device_id & 0xff;
      return true;
    }

    static UID AllDevices() {
      UID uid(0xffff, 0xffffffff);
      return uid;
    }

    static UID AllManufactureDevices(uint16_t esta_id) {
      UID uid(esta_id, 0xffffffff);
      return uid;
    }

    enum { UID_SIZE = 6 };

  private:
    struct rdm_uid {
      uint16_t esta_id;
      uint32_t device_id;
    };

    struct rdm_uid m_uid;

    int cmp(const UID &a, const UID &b) const {
      if (a.m_uid.esta_id == b.m_uid.esta_id)
        return cmp(a.m_uid.device_id, b.m_uid.device_id);
      return cmp(a.m_uid.esta_id, b.m_uid.esta_id);
    }

    int cmp(int a, int b) const {
      if (a == b)
        return 0;
      return a < b ? -1 : 1;
    }
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_UID_H_
