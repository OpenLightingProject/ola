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
 * OutputStream.h
 * A class that you can write structured data to.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_OUTPUTSTREAM_H_
#define INCLUDE_OLA_IO_OUTPUTSTREAM_H_

#include <stdint.h>
#include <ola/base/Macro.h>
#include <ola/io/OutputBuffer.h>

namespace ola {
namespace io {

/**
 * OutputStreamInterface. The Abstract base class for all OutputStreams.
 * Similar to ostream.
 */
class OutputStreamInterface {
 public:
    virtual ~OutputStreamInterface() {}

    // Append some data to this OutputQueue
    virtual void Write(const uint8_t *data, unsigned int length) = 0;

    virtual OutputStreamInterface& operator<<(uint8_t) = 0;
    virtual OutputStreamInterface& operator<<(uint16_t) = 0;
    virtual OutputStreamInterface& operator<<(uint32_t) = 0;
    virtual OutputStreamInterface& operator<<(int8_t) = 0;
    virtual OutputStreamInterface& operator<<(int16_t) = 0;
    virtual OutputStreamInterface& operator<<(int32_t) = 0;
};


/**
 * OutputStream. This writes data to an OutputBuffer
 */
class OutputStream: public OutputStreamInterface {
 public:
    // Ownership of the OutputBuffer is not transferred.
    explicit OutputStream(OutputBufferInterface *buffer)
        : m_buffer(buffer) {
    }
    virtual ~OutputStream() {}

    // Append some data directly to this OutputBuffer
    void Write(const uint8_t *data, unsigned int length) {
      m_buffer->Write(data, length);
    }

    OutputStream& operator<<(uint8_t val) { return Write(val); }
    OutputStream& operator<<(uint16_t val) { return Write(val); }
    OutputStream& operator<<(uint32_t val) { return Write(val); }
    OutputStream& operator<<(int8_t val) { return Write(val); }
    OutputStream& operator<<(int16_t val) { return Write(val); }
    OutputStream& operator<<(int32_t val) { return Write(val); }

 private:
    OutputBufferInterface *m_buffer;

    template<typename T>
    OutputStream& Write(const T &val) {
      m_buffer->Write(reinterpret_cast<const uint8_t*>(&val),
                      static_cast<unsigned int>(sizeof(val)));
      return *this;
    }

    DISALLOW_COPY_AND_ASSIGN(OutputStream);
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_OUTPUTSTREAM_H_
