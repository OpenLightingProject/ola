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
 * MemoryBlockPool.h
 * Allocates and Releases MemoryBlocks.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_MEMORYBLOCKPOOL_H_
#define INCLUDE_OLA_IO_MEMORYBLOCKPOOL_H_

#include <ola/Logging.h>
#include <ola/io/MemoryBlock.h>
#include <queue>

namespace ola {
namespace io {

/**
 * MemoryBlockPool. This class is not thread safe.
 */
class MemoryBlockPool {
 public:
    explicit MemoryBlockPool(unsigned int block_size = DEFAULT_BLOCK_SIZE)
        : m_block_size(block_size),
          m_blocks_allocated(0) {
    }
    ~MemoryBlockPool() {
      Purge();
    }

    // Allocate a new MemoryBlock from the pool. May return NULL if allocation
    // fails.
    MemoryBlock *Allocate() {
      if (m_free_blocks.empty()) {
        uint8_t* data = new uint8_t[m_block_size];
        OLA_DEBUG << "new block allocated at @" << reinterpret_cast<int*>(data);
        if (data) {
          m_blocks_allocated++;
          return new MemoryBlock(data, m_block_size);
        } else {
          return NULL;
        }
      } else {
        MemoryBlock *block = m_free_blocks.front();
        m_free_blocks.pop();
        return block;
      }
    }

    // Release a MemoryBlock back to the pool.
    void Release(MemoryBlock *block) {
      m_free_blocks.push(block);
    }

    // Returns the number of free blocks in the pool.
    unsigned int FreeBlocks() const {
      return static_cast<unsigned int>(m_free_blocks.size());
    }

    // Deletes all free blocks.
    void Purge() {
      Purge(0);
    }

    // Delete all but remaining free blocks.
    void Purge(unsigned int remaining) {
      while (m_free_blocks.size() != remaining) {
        MemoryBlock *block = m_free_blocks.front();
        m_blocks_allocated--;
        delete block;
        m_free_blocks.pop();
      }
    }

    unsigned int BlocksAllocated() const { return m_blocks_allocated; }

    // default to 1k blocks
    static const unsigned int DEFAULT_BLOCK_SIZE = 1024;

 private:
    std::queue<MemoryBlock*> m_free_blocks;
    const unsigned int m_block_size;
    unsigned int m_blocks_allocated;
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_MEMORYBLOCKPOOL_H_
