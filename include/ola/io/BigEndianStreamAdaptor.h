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
 * BigEndianStreamAdaptor.h
 * Wraps another {Input,Output}StreamInterface object and converts from Big
 * Endian to host order.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_BIGENDIANSTREAMADAPTOR_H_
#define INCLUDE_OLA_IO_BIGENDIANSTREAMADAPTOR_H_

#include <ola/io/InputStream.h>
#include <ola/io/OutputStream.h>
#include <ola/network/NetworkUtils.h>

namespace ola {
namespace io {

/**
 * BigEndianInputStreamAdaptor.
 * Wraps a InputStreamInterface object and converts from Big Endian to
 * host order.
 */
class BigEndianInputStreamAdaptor: public InputStreamInterface {
  public:
    // Ownership of the stream is not transferred.
    explicit BigEndianInputStreamAdaptor(InputStreamInterface *stream)
        : m_stream(stream) {
    }
    ~BigEndianInputStreamAdaptor() {}

    bool operator>>(int8_t &val) { return ExtractAndConvert(val); }
    bool operator>>(uint8_t &val) { return ExtractAndConvert(val); }
    bool operator>>(int16_t &val) { return ExtractAndConvert(val); }
    bool operator>>(uint16_t &val) { return ExtractAndConvert(val); }
    bool operator>>(int32_t &val) { return ExtractAndConvert(val); }
    bool operator>>(uint32_t &val) { return ExtractAndConvert(val); }

  private:
    InputStreamInterface *m_stream;

    template <typename T>
    bool ExtractAndConvert(T &val) {
      bool ok = (*m_stream) >> val;
      val = ola::network::NetworkToHost(val);
      return ok;
    }

    BigEndianInputStreamAdaptor(const BigEndianInputStreamAdaptor&);
    BigEndianInputStreamAdaptor& operator=(const BigEndianInputStreamAdaptor&);
};


/**
 * BigEndianOutputStreamAdaptor.
 * Wraps a OutputStreamInterface object and converts from Big Endian to
 * host order.
 */
class BigEndianOutputStreamAdaptor: public OutputStreamInterface {
  public:
    // Ownership of the stream is not transferred.
    explicit BigEndianOutputStreamAdaptor(OutputStreamInterface *stream)
        : m_stream(stream) {
    }
    ~BigEndianOutputStreamAdaptor() {}

    void Write(const uint8_t *data, unsigned int length) {
      m_stream->Write(data, length);
    }

    BigEndianOutputStreamAdaptor& operator<<(int8_t val) {
      return ConvertAndWrite(val);
    }

    BigEndianOutputStreamAdaptor& operator<<(uint8_t val) {
      return ConvertAndWrite(val);
    }

    BigEndianOutputStreamAdaptor& operator<<(int16_t val) {
      return ConvertAndWrite(val);
    }

    BigEndianOutputStreamAdaptor& operator<<(uint16_t val) {
      return ConvertAndWrite(val);
    }

    BigEndianOutputStreamAdaptor& operator<<(int32_t val) {
      return ConvertAndWrite(val);
    }

    BigEndianOutputStreamAdaptor& operator<<(uint32_t val) {
      return ConvertAndWrite(val);
    }

  private:
    OutputStreamInterface *m_stream;

    template <typename T>
    BigEndianOutputStreamAdaptor& ConvertAndWrite(T &val) {
      (*m_stream) << ola::network::HostToNetwork(val);
      return *this;
    }

    BigEndianOutputStreamAdaptor(const BigEndianOutputStreamAdaptor&);
    BigEndianOutputStreamAdaptor& operator=(
        const BigEndianOutputStreamAdaptor&);
};
}  // io
}  // ola
#endif  // INCLUDE_OLA_IO_BIGENDIANSTREAMADAPTOR_H_
