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
 * BigEndianInputStream.h
 * Read big-endian formatted data from a InputBuffer
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_BIGENDIANINPUTSTREAM_H_
#define INCLUDE_OLA_IO_BIGENDIANINPUTSTREAM_H_

#include <ola/io/InputStream.h>
#include <ola/network/NetworkUtils.h>

namespace ola {
namespace io {

/**
 * BigEndianInputStream.
 * Extracts formatted data from a InputBuffer, and converts it
 * from big endian to host format.
 */
class BigEndianInputStream: public InputStream {
  public:
    // Ownership of buffer is not transferred.
    explicit BigEndianInputStream(InputBuffer *buffer)
        : m_buffer(buffer) {
    }
    virtual ~BigEndianInputStream() {}

    bool operator>>(int8_t &val) {
      return Extract(val);
    }

    bool operator>>(uint8_t &val) {
      return Extract(val);
    }

    bool operator>>(int16_t &val) {
      return Extract(val);
    }

    bool operator>>(uint16_t &val) {
      return Extract(val);
    }

    bool operator>>(int32_t &val) {
      return Extract(val);
    }

    bool operator>>(uint32_t &val) {
      return Extract(val);
    }


  private:
    InputBuffer *m_buffer;

    template <typename T>
    bool Extract(T &val);

    BigEndianInputStream(const BigEndianInputStream&);
    BigEndianInputStream& operator=(const BigEndianInputStream&);
};

template <typename T>
bool BigEndianInputStream::Extract(T &val) {
  unsigned int length = sizeof(val);
  m_buffer->Get(reinterpret_cast<uint8_t*>(&val), &length);
  val = ola::network::NetworkToHost(val);
  return length == sizeof(val);
}
}  // io
}  // ola
#endif  // INCLUDE_OLA_IO_BIGENDIANINPUTSTREAM_H_
