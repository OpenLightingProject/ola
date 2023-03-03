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
 * BigEndianStream.h
 * Wraps another {Input,Output}StreamInterface object and converts from Big
 * Endian to host order.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_BIGENDIANSTREAM_H_
#define INCLUDE_OLA_IO_BIGENDIANSTREAM_H_

#include <ola/base/Macro.h>
#include <ola/io/InputStream.h>
#include <ola/io/OutputStream.h>
#include <ola/network/NetworkUtils.h>
#include <string>

namespace ola {
namespace io {

// An abstract class that guarantees byte order will be converted.
class BigEndianInputStreamInterface: public InputStreamInterface {};

/**
 * BigEndianInputStreamAdaptor.
 * Wraps a InputStreamInterface object and converts from Big Endian to
 * host order.
 */
class BigEndianInputStreamAdaptor: public BigEndianInputStreamInterface {
 public:
    // Ownership of the stream is not transferred.
    explicit BigEndianInputStreamAdaptor(InputStreamInterface *stream)
        : m_stream(stream) {
    }
    ~BigEndianInputStreamAdaptor() {}

    bool operator>>(int8_t &val) { return ExtractAndConvert(&val); }
    bool operator>>(uint8_t &val) { return ExtractAndConvert(&val); }
    bool operator>>(int16_t &val) { return ExtractAndConvert(&val); }
    bool operator>>(uint16_t &val) { return ExtractAndConvert(&val); }
    bool operator>>(int32_t &val) { return ExtractAndConvert(&val); }
    bool operator>>(uint32_t &val) { return ExtractAndConvert(&val); }

    unsigned int ReadString(std::string *output, unsigned int size) {
      return m_stream->ReadString(output, size);
    }

 private:
    InputStreamInterface *m_stream;

    template <typename T>
    bool ExtractAndConvert(T *val) {
      bool ok = (*m_stream) >> *val;
      *val = ola::network::NetworkToHost(*val);
      return ok;
    }

    DISALLOW_COPY_AND_ASSIGN(BigEndianInputStreamAdaptor);
};


/**
 * A Big Endian Input stream that wraps an InputBufferInterface
 */
class BigEndianInputStream: public BigEndianInputStreamInterface {
 public:
    // Ownership of the InputBuffer is not transferred.
    explicit BigEndianInputStream(InputBufferInterface *buffer)
        : m_input_stream(buffer),
          m_adaptor(&m_input_stream) {
    }
    ~BigEndianInputStream() {}

    bool operator>>(int8_t &val) { return m_adaptor >> val; }
    bool operator>>(uint8_t &val) { return m_adaptor >> val; }
    bool operator>>(int16_t &val) { return m_adaptor >> val; }
    bool operator>>(uint16_t &val) { return m_adaptor >> val; }
    bool operator>>(int32_t &val) { return m_adaptor >> val; }
    bool operator>>(uint32_t &val) { return m_adaptor >> val; }

    unsigned int ReadString(std::string *output, unsigned int size) {
      return m_adaptor.ReadString(output, size);
    }

 private:
    InputStream m_input_stream;
    BigEndianInputStreamAdaptor m_adaptor;

    DISALLOW_COPY_AND_ASSIGN(BigEndianInputStream);
};


// An abstract class that guarantees byte order will be converted.
class BigEndianOutputStreamInterface: public OutputStreamInterface {};

/**
 * BigEndianOutputStreamAdaptor.
 * Wraps a OutputStreamInterface object and converts from Big Endian to
 * host order.
 */
class BigEndianOutputStreamAdaptor: public BigEndianOutputStreamInterface {
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
    BigEndianOutputStreamAdaptor& ConvertAndWrite(const T &val) {
      (*m_stream) << ola::network::HostToNetwork(val);
      return *this;
    }

    DISALLOW_COPY_AND_ASSIGN(BigEndianOutputStreamAdaptor);
};


/**
 * A Big Endian Output stream that wraps an OutputBufferInterface
 */
class BigEndianOutputStream: public BigEndianOutputStreamInterface {
 public:
    // Ownership of the OutputBuffer is not transferred.
    explicit BigEndianOutputStream(OutputBufferInterface *buffer)
        : m_output_stream(buffer),
          m_adaptor(&m_output_stream) {
    }
    ~BigEndianOutputStream() {}

    void Write(const uint8_t *data, unsigned int length) {
      m_adaptor.Write(data, length);
    }

    BigEndianOutputStream& operator<<(int8_t val) { return Output(val); }
    BigEndianOutputStream& operator<<(uint8_t val) { return Output(val); }
    BigEndianOutputStream& operator<<(int16_t val) { return Output(val); }
    BigEndianOutputStream& operator<<(uint16_t val) { return Output(val); }
    BigEndianOutputStream& operator<<(int32_t val) { return Output(val); }
    BigEndianOutputStream& operator<<(uint32_t val) { return Output(val); }

 private:
    OutputStream m_output_stream;
    BigEndianOutputStreamAdaptor m_adaptor;

    template <typename T>
    BigEndianOutputStream& Output(T val) {
      m_adaptor << val;
      return *this;
    }

    DISALLOW_COPY_AND_ASSIGN(BigEndianOutputStream);
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_BIGENDIANSTREAM_H_
