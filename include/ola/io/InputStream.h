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
 * InputStream.h
 * A interface & class to extract formatted data.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_INPUTSTREAM_H_
#define INCLUDE_OLA_IO_INPUTSTREAM_H_

#include <stdint.h>
#include <ola/base/Macro.h>
#include <ola/io/InputBuffer.h>
#include <string>

namespace ola {
namespace io {

/**
 * InputStreamInterface. Similar to istream.
 */
class InputStreamInterface {
 public:
    virtual ~InputStreamInterface() {}

    /*
     * Extract data from the Stream.
     * Returns false if there was insufficient data to extract.
     */
    virtual bool operator>>(int8_t &val) = 0;
    virtual bool operator>>(uint8_t &val) = 0;
    virtual bool operator>>(int16_t &val) = 0;
    virtual bool operator>>(uint16_t &val) = 0;
    virtual bool operator>>(int32_t &val) = 0;
    virtual bool operator>>(uint32_t &val) = 0;

    /*
     * Append up to size bytes of data to the string.
     * @returns the number of bytes read
     */
    virtual unsigned int ReadString(std::string *output, unsigned int size) = 0;
};


/**
 * InputStream.
 * Extract formatted data from a InputBuffer.
 */
class InputStream: public InputStreamInterface {
 public:
    // Ownership of the InputBuffer is not transferred.
    explicit InputStream(InputBufferInterface *buffer)
        : m_buffer(buffer) {
    }
    virtual ~InputStream() {}

    bool operator>>(int8_t &val) { return Extract(&val); }
    bool operator>>(uint8_t &val) { return Extract(&val); }
    bool operator>>(int16_t &val) { return Extract(&val); }
    bool operator>>(uint16_t &val) { return Extract(&val); }
    bool operator>>(int32_t &val) { return Extract(&val); }
    bool operator>>(uint32_t &val) { return Extract(&val); }

    unsigned int ReadString(std::string *output, unsigned int size) {
      return m_buffer->Read(output, size);
    }

 private:
    InputBufferInterface *m_buffer;

    template <typename T>
    bool Extract(T *val) {
      unsigned int length = m_buffer->Read(reinterpret_cast<uint8_t*>(val),
                                           sizeof(*val));
      return length == sizeof(*val);
    }

    DISALLOW_COPY_AND_ASSIGN(InputStream);
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_INPUTSTREAM_H_
