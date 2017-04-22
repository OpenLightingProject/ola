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
 * PDU.cpp
 * The base PDU class
 * Copyright (C) 2007 Simon Newton
 */

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "libs/acn/PDU.h"

namespace ola {
namespace acn {

using ola::io::OutputStream;
using ola::network::HostToNetwork;

/*
 * Return the length of this PDU
 * @return length of the pdu
 */
unsigned int PDU::Size() const {
  unsigned int length = m_vector_size + HeaderSize() + DataSize();

  if (length > TWOB_LENGTH_LIMIT - 2)
    length += 1;
  length += 2;
  return length;
}


/*
 * Pack this PDU into a buffer
 * @param buffer pointer to the buffer
 * @param length length of the buffer
 * @return false on error, true otherwise
 */
bool PDU::Pack(uint8_t *buffer, unsigned int *length) const {
  unsigned int size = Size();
  unsigned int offset = 0;

  if (*length < size) {
    OLA_WARN << "PDU Pack: buffer too small, required " << size << ", got "
      << *length;
    *length = 0;
    return false;
  }

  if (size <= TWOB_LENGTH_LIMIT) {
    buffer[0] = (uint8_t) ((size & 0x0f00) >> 8);
    buffer[1] = (uint8_t) (size & 0xff);
  } else {
    buffer[0] = (uint8_t) ((size & 0x0f0000) >> 16);
    buffer[1] = (uint8_t) ((size & 0xff00) >> 8);
    buffer[2] = (uint8_t) (size & 0xff);
    offset += 1;
  }

  buffer[0] |= VFLAG_MASK;
  buffer[0] |= HFLAG_MASK;
  buffer[0] |= DFLAG_MASK;
  offset += 2;

  switch (m_vector_size) {
    case PDU::ONE_BYTE:
      buffer[offset++] = (uint8_t) m_vector;
      break;
    case PDU::TWO_BYTES:
      buffer[offset++] = static_cast<uint8_t>(0xff & (m_vector >> 8));
      buffer[offset++] = static_cast<uint8_t>(0xff & m_vector);
      break;
    case PDU::FOUR_BYTES:
      buffer[offset++] = static_cast<uint8_t>(0xff & (m_vector >> 24));
      buffer[offset++] = static_cast<uint8_t>(0xff & (m_vector >> 16));
      buffer[offset++] = static_cast<uint8_t>(0xff & (m_vector >> 8));
      buffer[offset++] = static_cast<uint8_t>(0xff & m_vector);
      break;
    default:
      OLA_WARN << "unknown vector size " << m_vector_size;
      return false;
  }

  unsigned int bytes_used = *length - offset;
  if (!PackHeader(buffer + offset, &bytes_used)) {
    *length = 0;
    return false;
  }
  offset += bytes_used;

  bytes_used = *length - offset;
  if (!PackData(buffer + offset, &bytes_used)) {
    *length = 0;
    return false;
  }
  offset += bytes_used;
  *length = offset;
  return true;
}


/**
 * Write this PDU to an OutputStream.
 */
void PDU::Write(OutputStream *stream) const {
  unsigned int size = Size();

  if (size <= TWOB_LENGTH_LIMIT) {
    uint16_t flags_and_length = static_cast<uint16_t>(size);
    flags_and_length |= (VFLAG_MASK | HFLAG_MASK | DFLAG_MASK) << 8u;
    *stream << HostToNetwork(flags_and_length);
  } else {
    uint8_t vhl_flags = static_cast<uint8_t>((size & 0x0f0000) >> 16);
    vhl_flags |= VFLAG_MASK | HFLAG_MASK | DFLAG_MASK;
    *stream << vhl_flags;
    *stream << (uint8_t) ((size & 0xff00) >> 8);
    *stream << (uint8_t) (size & 0xff);
  }

  switch (m_vector_size) {
    case PDU::ONE_BYTE:
      *stream << static_cast<uint8_t>(m_vector);
      break;
    case PDU::TWO_BYTES:
      *stream << HostToNetwork(static_cast<uint16_t>(m_vector));
      break;
    case PDU::FOUR_BYTES:
      *stream << HostToNetwork(m_vector);
      break;
  }

  PackHeader(stream);
  PackData(stream);
}


/**
 * Prepend the flags and length to an OutputBufferInterface.
 */
void PDU::PrependFlagsAndLength(ola::io::OutputBufferInterface *output,
                                uint8_t flags) {
  PrependFlagsAndLength(output, output->Size(), flags);
}


/**
 * Prepend the flags and length to an OutputBufferInterface.
 */
void PDU::PrependFlagsAndLength(ola::io::OutputBufferInterface *output,
                                unsigned int size,
                                uint8_t flags) {
  if (size + 2 <= TWOB_LENGTH_LIMIT) {
    size += 2;
    uint16_t flags_and_length = static_cast<uint16_t>(size);
    flags_and_length |= static_cast<uint16_t>(flags << 8u);
    flags_and_length = HostToNetwork(flags_and_length);
    output->Write(reinterpret_cast<uint8_t*>(&flags_and_length),
                  sizeof(flags_and_length));
  } else {
    size += 3;
    uint8_t flags_and_length[3];
    flags_and_length[0] = static_cast<uint8_t>((size & 0x0f0000) >> 16);
    flags_and_length[0] |= flags;
    flags_and_length[1] = static_cast<uint8_t>((size & 0xff00) >> 8);
    flags_and_length[2] = static_cast<uint8_t>(size & 0xff);
    output->Write(flags_and_length, sizeof(flags_and_length));
  }
}
}  // namespace acn
}  // namespace ola
