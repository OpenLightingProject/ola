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
 * MemoryBuffer.h
 * A derived class of InputBuffer that wraps a memory buffer.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_MEMORYBUFFER_H_
#define INCLUDE_OLA_IO_MEMORYBUFFER_H_

#include <ola/io/InputBuffer.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <string>

namespace ola {
namespace io {

/**
 * Wraps a block of memory and presents the InputBuffer interface. This does
 * not free the memory when the object is destroyed.
 */
class MemoryBuffer: public InputBufferInterface {
 public:
    explicit MemoryBuffer(const uint8_t *data, unsigned int size)
        : m_data(data),
          m_size(size),
          m_cursor(0) {
    }
    ~MemoryBuffer() {}

    unsigned int Read(uint8_t *data, unsigned int length) {
      unsigned int data_size = std::min(m_size - m_cursor, length);
      memcpy(data, m_data + m_cursor, data_size);
      m_cursor += data_size;
      return data_size;
    }

    unsigned int Read(std::string *output, unsigned int length) {
      unsigned int data_size = std::min(m_size - m_cursor, length);
      output->append(reinterpret_cast<const char*>(m_data + m_cursor),
                     data_size);
      m_cursor += data_size;
      return data_size;
    }

 private:
    const uint8_t *m_data;
    const unsigned int m_size;
    unsigned int m_cursor;

    MemoryBuffer(const MemoryBuffer&);
    MemoryBuffer& operator=(const MemoryBuffer&);
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_MEMORYBUFFER_H_
