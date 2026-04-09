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
 * MemoryBlock.h
 * A block of memory.
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @file MemoryBlock.h
 * @brief Wraps a memory region.
 */

#ifndef INCLUDE_OLA_IO_MEMORYBLOCK_H_
#define INCLUDE_OLA_IO_MEMORYBLOCK_H_

#include <stdint.h>
#include <string.h>
#include <algorithm>

namespace ola {
namespace io {

/**
 * @class MemoryBlock ola/io/MemoryBlock.h
 * @brief A MemoryBlock encapsulates a chunk of memory. It's used by the
 * IOQueue and IOStack classes.
 */
class MemoryBlock {
 public:
    /**
     * @brief Construct a new MemoryBlock.
     * @param data a pointer to the memory region to use, ownership is
     * transferred.
     * @param size the size of the memory region to use
     */
    MemoryBlock(uint8_t *data, unsigned int size)
        : m_data(data),
          m_data_end(data + size),
          m_capacity(size),
          m_first(data),
          m_last(data) {
    }

    /**
     * @brief Destructor, this frees the memory for the block.
     */
    ~MemoryBlock() {
      delete[] m_data;
    }

    /**
     * @brief Move the insertation point to the end of the block.
     * This is useful if you want to use the block in prepend mode.
     */
    void SeekBack() {
      m_first = m_data_end;
      m_last = m_first;
    }

    /**
     * @brief The size of the memory region for this block.
     * @returns the size of the memory region for this block.
     */
    unsigned int Capacity() const { return m_capacity; }

    /**
     * @brief The free space at the end of the block.
     * @returns the free space at the end of the block.
     */
    unsigned int Remaining() const {
      return static_cast<unsigned int>(m_data_end - m_last);
    }

    /**
     * @brief The size of data in this block.
     * @returns the size of the data in this block.
     */
    unsigned int Size() const {
      return static_cast<unsigned int>(m_last - m_first);
    }

    /**
     * @brief Check if the block contains data
     * @returns true if the block contains no data, false otherwise.
     */
    bool Empty() const {
      return m_last == m_first;
    }

    /**
     * @brief Provides a pointer to the first byte of valid data in this block.
     * @returns a pointer to the first byte of valid data in this block.
     */
    uint8_t *Data() const { return m_first; }

    /**
     * @brief Append data to this block.
     * @param data the data to append.
     * @param length the length of the data to append.
     * @returns the number of bytes written, which will be less than length if
     * the block is now full.
     */
    unsigned int Append(const uint8_t *data, unsigned int length) {
      unsigned int bytes_to_write = std::min(
          length, static_cast<unsigned int>(m_data_end - m_last));
      memcpy(m_last, data, bytes_to_write);
      m_last += bytes_to_write;
      return bytes_to_write;
    }

    /**
     * @brief Prepend data to this block.
     * @param data the data to prepend.
     * @param length the length of the data to prepend.
     * @returns the amount of data prepended, (from the end of data), which
     * will be less than length if the block is now full.
     */
    unsigned int Prepend(const uint8_t *data, unsigned int length) {
      unsigned int bytes_to_write = std::min(
          length, static_cast<unsigned int>(m_first - m_data));
      memcpy(m_first - bytes_to_write, data + length - bytes_to_write,
             bytes_to_write);
      m_first -= bytes_to_write;
      return bytes_to_write;
    }

    /**
     * Copy data from the MemoryBlock into the location provided.
     * @param[out] data a pointer to the memory to copy the block data to.
     * @param length the maximum size of data to copy.
     * @returns the amount of data copied
     */
    unsigned int Copy(uint8_t *data, unsigned int length) const {
      unsigned int bytes_to_read = std::min(
          length, static_cast<unsigned int>(m_last - m_first));
      memcpy(data, m_first, bytes_to_read);
      return bytes_to_read;
    }

    /**
     * @brief Remove data from the front of the block
     * @param length the amount of data to remove
     * @returns the amount of data removed.
     */
    unsigned int PopFront(unsigned int length) {
      unsigned int bytes_to_pop = std::min(
          length, static_cast<unsigned int>(m_last - m_first));
      m_first += bytes_to_pop;
      // reset
      if (m_first == m_last) {
        m_first = m_data;
        m_last = m_data;
      }
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
