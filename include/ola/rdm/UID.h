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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * UID.h
 * Representation of an RDM UID
 * Copyright (C) 2005 Simon Newton
 */

/**
 * @addtogroup rdm_uid
 * @{
 * @file UID.h
 * @brief A RDM unique identifier (UID).
 * @}
 */
#ifndef INCLUDE_OLA_RDM_UID_H_
#define INCLUDE_OLA_RDM_UID_H_

#include <stdint.h>

#include <ola/util/Utils.h>

#include <iomanip>
#include <sstream>
#include <string>

namespace ola {
namespace rdm {

/**
 * @addtogroup rdm_uid
 * @{
 * @class UID
 * @brief Represents a RDM UID.
 *
 * UIDs are 6 bytes, the first two bytes are the
 * [manufacturer code](http://tsp.esta.org/tsp/working_groups/CP/mfctrIDs.php)
 * and the last 4 bytes are the device id. UIDs are written as:
 *
 * @code
 *   XXXX:YYYYYYYY
 * @endcode
 * @}
 */
class UID {
 public:
    enum { LENGTH = 6 };

    /**
     * @brief Constructs a new UID
     * @param esta_id the ESTA (manufacturer ID).
     * @param device_id the device ID.
     */
    UID(uint16_t esta_id, uint32_t device_id) {
      m_uid.esta_id = esta_id;
      m_uid.device_id = device_id;
    }

    /**
     * @brief Constructs a new UID from a uint64_t
     * @param uid a uint64_t in the form 0x0000XXXXYYYYYYYY.
     */
    explicit UID(uint64_t uid) {
      m_uid.esta_id = static_cast<uint16_t>(uid >> 32);
      m_uid.device_id = static_cast<uint32_t>(uid);
    }

    /**
     * @brief Copy constructor.
     * @param uid the UID to copy.
     */
    UID(const UID &uid) {
      m_uid.esta_id = uid.m_uid.esta_id;
      m_uid.device_id = uid.m_uid.device_id;
    }

    /**
     * @brief Construct a new UID from binary data.
     * @param data a pointer to the memory containing the UID data. The data
     * should be most significant byte first.
     */
    explicit UID(const uint8_t *data) {
      m_uid.esta_id = ola::utils::JoinUInt8(data[0], data[1]);
      m_uid.device_id = ola::utils::JoinUInt8(data[2], data[3], data[4],
                                              data[5]);
    }

    /**
     * @brief Assignment operator
     */
    UID& operator=(const UID& other) {
      if (this != &other) {
        m_uid.esta_id = other.m_uid.esta_id;
        m_uid.device_id = other.m_uid.device_id;
      }
      return *this;
    }

    /**
     * @brief Equality operator.
     * @param other the UID to compare to.
     */
    bool operator==(const UID &other) const {
      return 0 == cmp(*this, other);
    }

    /**
     * @brief Inequality operator.
     * @param other the UID to compare to.
     */
    bool operator!=(const UID &other) const {
      return 0 != cmp(*this, other);
    }

    /**
     * @brief Greater than.
     * @param other the UID to compare to.
     */
    bool operator>(const UID &other) const {
      return cmp(*this, other) > 0;
    }

    /**
     * @brief Less than.
     * @param other the UID to compare to.
     */
    bool operator<(const UID &other) const {
      return cmp(*this, other) < 0;
    }

    /**
     * @brief Greater than or equal to.
     * @param other the UID to compare to.
     */
    bool operator>=(const UID &other) const {
      return cmp(*this, other) >= 0;
    }

    /**
     * @brief Less than or equal to.
     * @param other the UID to compare to.
     */
    bool operator<=(const UID &other) const {
      return cmp(*this, other) <= 0;
    }

    /**
     * @brief The manufacturer ID for this UID
     * @returns the manufacturer id for this UID.
     */
    uint16_t ManufacturerId() const { return m_uid.esta_id; }

    /**
     * @brief The device ID for this UID
     * @returns the device id for this UID.
     */
    uint32_t DeviceId() const { return m_uid.device_id; }

    /**
     * @brief Check if this UID is a broadcast or vendorcast UID.
     * @returns true if the device id is 0xffffffff.
     */
    bool IsBroadcast() const { return m_uid.device_id == ALL_DEVICES; }

    /**
     * @brief Check if this UID matches against another.
     * @param uid the UID to check against
     * @returns true if the UIDs are equal or if this UID is a broadcast UID
     * and the uid argument falls within the broadcast range.
     *
     * This is useful to determine if a responder should reply to a message.
     *
     * @examplepara
     * @code
     *   UID uid(0x7a70, 1);
     *   uid.DirectedToUID(uid);  // always true.
     *
     *   UID broadcast_uid(UID::AllDevices());
     *   broadcast_uid.DirectedToUID(uid);  // true
     *
     *   UID vendorcast_uid(UID::VendorcastAddress(0x7a70));
     *   vendorcast_uid.DirectedToUID(uid);  // true
     *
     *   UID other_vendorcast_uid(UID::VendorcastAddress(0x0808));
     *   other_vendorcast_uid.DirectedToUID(uid);  // false
     * @endcode
     */
    bool DirectedToUID(const UID &uid) const {
      return *this == uid ||
            (IsBroadcast() &&
             (ManufacturerId() == ALL_MANUFACTURERS ||
              ManufacturerId() == uid.ManufacturerId()));
    }

    /**
     * @brief Convert a UID to a uint64_t.
     * @returns a uint64_t in the form 0x0000XXXXYYYYYYYY.
     */
    uint64_t ToUInt64() const {
      return ((static_cast<uint64_t>(m_uid.esta_id) << 32) + m_uid.device_id);
    }

    /**
     * @brief Convert a UID to a human readable string.
     * @returns a string in the form XXXX:YYYYYYYY.
     */
    std::string ToString() const {
      std::ostringstream str;
      str << std::setfill('0') << std::setw(4) << std::hex << m_uid.esta_id
        << ":" << std::setw(8) << m_uid.device_id;
      return str.str();
    }

    /**
     * @brief A helper function to write a UID to an ostream.
     * @param out the ostream
     * @param uid the UID to write.
     */
    friend std::ostream& operator<< (std::ostream &out, const UID &uid) {
      return out << uid.ToString();
    }

    /**
     * @brief Write the binary representation of the UID to memory.
     * @param buffer a pointer to memory to write the UID to
     * @param length the size of the memory block, should be at least UID_SIZE.
     * @returns true if length was >= UID_SIZE, false otherwise.
     */
    bool Pack(uint8_t *buffer, unsigned int length) const {
      if (length < UID_SIZE)
        return false;
      buffer[0] = static_cast<uint8_t>(m_uid.esta_id >> 8);
      buffer[1] = static_cast<uint8_t>(m_uid.esta_id & 0xff);
      buffer[2] = static_cast<uint8_t>(m_uid.device_id >> 24);
      buffer[3] = static_cast<uint8_t>(m_uid.device_id >> 16);
      buffer[4] = static_cast<uint8_t>(m_uid.device_id >> 8);
      buffer[5] = static_cast<uint8_t>(m_uid.device_id & 0xff);
      return true;
    }

    /**
     * @brief Returns a UID that matches all devices (ffff:ffffffff).
     * @returns a UID(0xffff, 0xffffffff).
     */
    static UID AllDevices() {
      return UID(ALL_MANUFACTURERS, ALL_DEVICES);
    }

    /**
     * @brief Returns a UID that matches all devices for a particular
     * manufacturer.
     * @param esta_id the manufacturer id of the devices to match.
     * @returns a UID(X, 0xffffffff).
     */
    static UID VendorcastAddress(uint16_t esta_id) {
      return UID(esta_id, ALL_DEVICES);
    }

    /**
     * @brief Returns a UID that matches all devices for a particular
     *     manufacturer.
     * @param uid a UID whose manufacturer id you want to match.
     * @returns a UID(X, 0xffffffff).
     */
    static UID VendorcastAddress(UID uid) {
      return UID(uid.ManufacturerId(), ALL_DEVICES);
    }

    /**
     * @brief Return a new UID from a string.
     * @param uid the UID as a string i.e. XXXX:YYYYYYYY.
     * @return a new UID object, or NULL if the string is not a valid UID.
     * Ownership of the new UID object is transferred to the caller.
     */
    static UID* FromString(const std::string &uid);

    /**
     * The size of a UID.
     */
    enum {
      UID_SIZE = LENGTH  /**< The size of a UID in binary form */
    };

    /**
     * @brief The value for the 'all manufacturers' id.
     */
    static const uint16_t ALL_MANUFACTURERS = 0xffff;

    /**
     * @brief The value for the 'all devices' id.
     */
    static const uint32_t ALL_DEVICES = 0xffffffff;

 private:
    struct rdm_uid {
      uint16_t esta_id;
      uint32_t device_id;
    };

    struct rdm_uid m_uid;

    int cmp(const UID &a, const UID &b) const {
      if (a.m_uid.esta_id == b.m_uid.esta_id) {
        return cmp(a.m_uid.device_id, b.m_uid.device_id);
      }
      return cmp(a.m_uid.esta_id, b.m_uid.esta_id);
    }

    int cmp(uint32_t a, uint32_t b) const {
      if (a == b) {
        return 0;
      }
      return a < b ? -1 : 1;
    }
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_UID_H_
