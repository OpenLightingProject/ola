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
 * BigEndianInputStreamAdaptor.h
 * Wraps another InputStreamInterface object and converts from Big Endian to
 * host order.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_BIGENDIANINPUTSTREAM_H_
#define INCLUDE_OLA_IO_BIGENDIANINPUTSTREAM_H_

#include <ola/io/InputStream.h>
#include <ola/network/NetworkUtils.h>

namespace ola {
namespace io {

/**
 * BigEndianInputStreamAdaptor.
 * Wraps another InputStreamInterface object and converts from Big Endian to
 * host order.
 */
class BigEndianInputStreamAdaptor: public InputStreamInterface {
  public:
    // Ownership of buffer is not transferred.
    explicit BigEndianInputStreamAdaptor(InputStreamInterface *stream)
        : m_stream(stream) {
    }
    virtual ~BigEndianInputStreamAdaptor() {}

    bool operator>>(int8_t &val) {
      return ExtractAndConvert(val);
    }

    bool operator>>(uint8_t &val) {
      return ExtractAndConvert(val);
    }

    bool operator>>(int16_t &val) {
      return ExtractAndConvert(val);
    }

    bool operator>>(uint16_t &val) {
      return ExtractAndConvert(val);
    }

    bool operator>>(int32_t &val) {
      return ExtractAndConvert(val);
    }

    bool operator>>(uint32_t &val) {
      return ExtractAndConvert(val);
    }


  private:
    InputStreamInterface *m_stream;

    template <typename T>
    bool ExtractAndConvert(T &val);

    BigEndianInputStreamAdaptor(const BigEndianInputStreamAdaptor&);
    BigEndianInputStreamAdaptor& operator=(const BigEndianInputStreamAdaptor&);
};

template <typename T>
bool BigEndianInputStreamAdaptor::ExtractAndConvert(T &val) {
  bool ok = (*m_stream) >> val;
  val = ola::network::NetworkToHost(val);
  return ok;
}
}  // io
}  // ola
#endif  // INCLUDE_OLA_IO_BIGENDIANINPUTSTREAM_H_
