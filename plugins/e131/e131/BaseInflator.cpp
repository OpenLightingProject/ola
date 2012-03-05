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
 * BaseInflator.cpp
 * The BaseInflator, other Inflators inherit from this one.
 * Copyright (C) 2007-2009 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <algorithm>
#include <map>
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/e131/e131/BaseInflator.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::NetworkToHost;

/*
 * Setup the base inflator
 */
BaseInflator::BaseInflator(PDU::vector_size v_size)
    : m_last_vector(0),
      m_vector_set(false),
      m_vector_size(v_size) {
}


/*
 * Set the inflator for a particular protocol
 * @param inflator a inflator
 * @return true if added, false if an inflator with this id already exists.
 */
bool BaseInflator::AddInflator(BaseInflator *inflator) {
  if (m_proto_map.find(inflator->Id()) == m_proto_map.end()) {
    m_proto_map[inflator->Id()] = inflator;
    return true;
  }
  return false;
}


/*
 * Get the current inflator for a protocol
 * @param proto the proto ID
 * @return the inflator for this protocol, or NULL if there isn't one set.
 */
BaseInflator *BaseInflator::GetInflator(uint32_t proto) const {
  std::map<uint32_t, class BaseInflator*>::const_iterator iter;
  iter = m_proto_map.find(proto);
  if (iter == m_proto_map.end())
    return NULL;
  return iter->second;
}


/*
 * Parse a block of PDUs
 * @param headers the HeaderSet for this PDU
 * @param data pointer to the data
 * @param length length of the data
 * @returns the amount of data used
 */
unsigned int BaseInflator::InflatePDUBlock(HeaderSet &headers,
                                           const uint8_t *data,
                                           unsigned int length) {
  unsigned int offset = 0;
  ResetPDUFields();

  if (length == 0)
    return 0;

  do {
    unsigned int bytes_used = 0;
    unsigned int pdu_length = 0;
    if (!DecodeLength(data + offset, length - offset, pdu_length, bytes_used))
      return offset;

    if (offset + pdu_length <= length) {
      InflatePDU(headers, data[offset], data + offset + bytes_used,
                 pdu_length - bytes_used);
    }
    offset += pdu_length;
  } while (offset < length);
  return std::min(offset, length);
}


/*
 * Reset the pdu fields
 */
void BaseInflator::ResetPDUFields() {
  m_vector_set = false;
  ResetHeaderField();
}


/*
 * Fetch the length from a pdu header
 * @param data  pointer to the head of the PDU
 * @param length length of the PDU data
 * @param pdu_length set to the length of the PDU
 * @param bytes_used set to the number of bytes used for the length field
 * @return true if everything worked, false if invalid data was found
 */
bool BaseInflator::DecodeLength(const uint8_t *data,
                                unsigned int length,
                                unsigned int &pdu_length,
                                unsigned int &bytes_used) const {
  uint8_t flags = data[0];
  if (!length) {
    bytes_used = 0;
    pdu_length = 0;
    return false;
  }

  if (flags & LFLAG_MASK) {
    if (length < 3) {
      OLA_WARN << "PDU length " << length << " < 3 and the LENGTH bit is set";
      return false;
    }
    bytes_used = 3;
    pdu_length =(data[2] +
        static_cast<unsigned int>(data[1] << 8) +
        static_cast<unsigned int>((data[0] & LENGTH_MASK) << 16));
  } else {
    if (length < 2) {
      OLA_WARN << "PDU length " << length << " < 2";
      return false;
    }
    bytes_used = 2;
    pdu_length = data[1] + static_cast<unsigned int>(
        (data[0] & LENGTH_MASK) << 8);
  }
  if (pdu_length < bytes_used) {
    OLA_WARN << "PDU length was set to " << pdu_length << " but " << bytes_used
      << " bytes were used in the header";
    bytes_used = 0;
    return false;
  }
  return true;
}


/*
 * Decode the vector field
 * @param flags the PDU flags
 * @param data pointer to the pdu data
 * @param length length of the data
 * @param vector the result of the vector
 * @param bytes_used the number of bytes consumed
 */
bool BaseInflator::DecodeVector(uint8_t flags, const uint8_t *data,
                                unsigned int length, uint32_t &vector,
                                unsigned int &bytes_used) {
  if (flags & PDU::VFLAG_MASK) {
    if ((unsigned int) m_vector_size > length) {
      vector = 0;
      bytes_used = 0;
      return false;
    }

    switch (m_vector_size) {
      case PDU::ONE_BYTE:
        vector = *data;
        break;
      case PDU::TWO_BYTES:
        vector = data[1] + static_cast<unsigned int>(data[0] << 8);
        break;
      case PDU::FOUR_BYTES:
        // careful: we can't cast to a uint32 because this isn't word aligned
        vector = data[3] +
          static_cast<unsigned int>(data[2] << 8) +
          static_cast<unsigned int>(data[1] << 16) +
          static_cast<unsigned int>(data[0] << 24);
        break;
      default:
        OLA_WARN << "unknown vector size " << m_vector_size;
        return false;
    }
    m_vector_set = true;
    bytes_used = m_vector_size;
    m_last_vector = vector;
  } else {
    bytes_used = 0;
    if (m_vector_set) {
      vector = m_last_vector;
    } else {
      vector = 0;
      bytes_used = 0;
      OLA_WARN << "Vector not set and no field to inherit from";
      return false;
    }
  }
  return true;
}


/*
 * Parse a generic PDU structure
 * @param headers a reference to the header set
 * @param flags the flag field
 * @param data  pointer to the vector field
 * @param pdu_len   length of the PDU
 * @return true if we inflated without errors
 */
bool BaseInflator::InflatePDU(HeaderSet &headers,
                              uint8_t flags,
                              const uint8_t *data,
                              unsigned int pdu_len) {
  uint32_t vector;
  unsigned int data_offset, header_bytes_used;
  bool result;

  if (!DecodeVector(flags, data, pdu_len, vector, data_offset))
    return false;

  if (flags & PDU::HFLAG_MASK) {
    result = DecodeHeader(headers, data + data_offset,
                          pdu_len - data_offset,
                          header_bytes_used);
  } else {
    result = DecodeHeader(headers, NULL, 0, header_bytes_used);
    header_bytes_used = 0;
  }
  if (!result)
    return false;

  if (!PostHeader(vector, headers))
    return true;

  // TODO(simon): handle the crazy DFLAG here

  data_offset += header_bytes_used;
  BaseInflator *inflator = GetInflator(vector);
  if (inflator) {
    return inflator->InflatePDUBlock(headers, data + data_offset,
                                     pdu_len - data_offset);
  } else {
    return HandlePDUData(vector, headers, data + data_offset,
                         pdu_len - data_offset);
  }
}


/*
 * The Post header hook. This is called once the header has been decoded but
 * before either the next inflator or handle_data is called.
 * @return false will cease processing this PDU
 */
bool BaseInflator::PostHeader(uint32_t vector, HeaderSet &headers) {
  (void) vector;
  (void) headers;
  return true;
}


/*
 * The base handle data method - does nothing
 */
bool BaseInflator::HandlePDUData(uint32_t vector,
                                 HeaderSet &headers,
                                 const uint8_t *data,
                                 unsigned int pdu_length) {
  OLA_WARN << "In BaseInflator::HandlePDUData, someone forgot to add" <<
    " a handler, vector id " << vector;
  (void) headers;
  (void) data;
  (void) pdu_length;
  return false;
}
}  // e131
}  // plugin
}  // ola
