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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * MemoryBlock.h
 * A block of memory.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_MEMORYBLOCK_H_
#define INCLUDE_OLA_IO_MEMORYBLOCK_H_

#include <stdint.h>
#include <string.h>
#include <algorithm>

namespace ola {
namespace io {

/**
 * A MemoryBlock encapsulates a chunk of memory. It's used within the IOQueue
 * and IOStack classes.
 */
class MemoryBlock {
  public:
    // Ownership of the data is transferred.
    MemoryBlock(uint8_t *data, unsigned int size)
        : m_data(data),
          m_data_end(data + size),
          m_capacity(size),
          m_first(data),
          m_last(data) {
    }

    ~MemoryBlock() {
      delete m_data;
    }

    // Move the insertion point to the end of the block. This is useful if you
    // want to use the block in pre-pend mode
    void SeekBack() {
      m_first = m_data_end;
      m_last = m_first;
    }

    // The amount of memory space in this block.
    unsigned int Capacity() const { return m_capacity; }

    // The amount of data in this block.
    unsigned int Size() const {
      return static_cast<unsigned int>(m_last - m_first);
    }

    bool Empty() const {
      return m_last == m_first;
    }

    // Pointer to the first byte of valid data in this block.
    uint8_t *Data() const { return m_first; }

    // Attempt to append the data to this block. returns the number of bytes
    // written, which will be less than length if the block is now full.
    unsigned int Append(const uint8_t *data, unsigned int length) {
      unsigned int bytes_to_write = std::min(
          length, static_cast<unsigned int>(m_data_end - m_last));
      memcpy(m_last, data, bytes_to_write);
      m_last += bytes_to_write;
      return bytes_to_write;
    }

    // Attempt to prepend the data to this block. returns the number of bytes
    // written (from the end of data), which will be less than length if the
    // block is now full.
    unsigned int Prepend(const uint8_t *data, unsigned int length) {
      unsigned int bytes_to_write = std::min(
          length, static_cast<unsigned int>(m_first - m_data));
      memcpy(m_first - bytes_to_write, data + length - bytes_to_write,
             bytes_to_write);
      m_first -= bytes_to_write;
      return bytes_to_write;
    }

    // Copy at most length bytes of data into the location data. This does not
    // change the data in any way.
    unsigned int Copy(uint8_t *data, unsigned int length) const {
      unsigned int bytes_to_read = std::min(
          length, static_cast<unsigned int>(m_last - m_first));
      memcpy(data, m_first, bytes_to_read);
      return bytes_to_read;
    }

    // Remove at most length bytes from the front of this block.
    unsigned int PopFront(unsigned int length) {
      unsigned int bytes_to_pop = std::min(
          length, static_cast<unsigned int>(m_last - m_first));
      m_first += bytes_to_pop;
      return bytes_to_pop;
    }

  private:
    uint8_t* const m_data;
    uint8_t* const m_data_end;
    unsigned int m_capacity;
    // Points to the valid data in the block.
    uint8_t *m_first;
    uint8_t *m_last;  // points to one after last
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_MEMORYBLOCK_H_
