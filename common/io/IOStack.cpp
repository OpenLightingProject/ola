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
 * IOStack.cpp
 * A non-contigous memory buffer
 * Copyright (C) 2012 Simon Newton
 */


#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/io/IOQueue.h>
#include <ola/io/IOStack.h>
#include <ola/io/MemoryBlockPool.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <queue>
#include <string>

namespace ola {
namespace io {

using std::min;


/**
 * IOStack.
 * @param block_size the size of blocks to use.
 */
IOStack::IOStack()
    : m_pool(new MemoryBlockPool()),
      m_delete_pool(true) {
}


IOStack::IOStack(MemoryBlockPool *block_pool)
    : m_pool(block_pool),
      m_delete_pool(false) {
}

/**
 * Clean up
 */
IOStack::~IOStack() {
  // Return all the blocks to the pool.
  BlockVector::iterator iter = m_blocks.begin();
  for (; iter != m_blocks.end(); ++iter)
    m_pool->Release(*iter);

  if (m_delete_pool)
    delete m_pool;
}


/**
 * Return the amount of data in the buffer
 */
unsigned int IOStack::Size() const {
  if (m_blocks.empty())
    return 0;

  unsigned int size = 0;
  BlockVector::const_iterator iter = m_blocks.begin();
  for (; iter != m_blocks.end(); ++iter)
    size += (*iter)->Size();
  return size;
}


/**
 * Append (length) bytes of data to the front of the buffer.
 */
void IOStack::Write(const uint8_t *data, unsigned int length) {
  if (m_blocks.empty()) {
    PrependBlock();
  }

  unsigned int bytes_written = 0;
  while (true) {
    bytes_written += m_blocks.front()->Prepend(data, length - bytes_written);
    if (bytes_written == length) {
      return;
    }
    PrependBlock();
  }
}


/**
 * Return this IOStack as an array of iovec structures.
 * Note: The iovec array points at internal memory structures. This array is
 * invalidated when any non-const methods are called (Append, Pop etc.)
 *
 * Is the IOStack is empty, this will return NULL and set iocnt to 0.
 *
 * Use FreeIOVec() to release the iovec array.
 */
const struct iovec *IOStack::AsIOVec(int *iocnt) const {
  if (m_blocks.empty()) {
    *iocnt = 0;
    return NULL;
  }

  int max_number_of_blocks = m_blocks.size();
  int number_of_blocks = 0;

  struct iovec *vector = new struct iovec[max_number_of_blocks];
  struct iovec *ptr = vector;
  BlockVector::const_iterator iter = m_blocks.begin();
  for (; iter != m_blocks.end(); ++iter, ++ptr, number_of_blocks++) {
    ptr->iov_base = (*iter)->Data();
    ptr->iov_len = (*iter)->Size();
  }

  *iocnt = number_of_blocks;
  return vector;
}


/**
 * Append the memory blocks in this stack to the IOQueue. This transfers
 * ownership of the MemoryBlocks to the queue, so the IOQueue and IOStack
 * should have the same MemoryBlockPool (or at the very least, the same
 * implementation).
 */
void IOStack::MoveToIOQueue(IOQueue *queue) {
  BlockVector::const_iterator iter = m_blocks.begin();
  for (; iter != m_blocks.end(); ++iter) {
    queue->AppendBlock(*iter);
  }
  m_blocks.clear();
}


/**
 * Free a iovec structure
 */
void IOStack::FreeIOVec(const struct iovec *iov) {
  if (iov)
    delete[] iov;
}

void IOStack::Purge() {
  m_pool->Purge();
}

/**
 * Dump this IOStack as a human readable string
 */
void IOStack::Dump(std::ostream *output) {
  // For now just alloc memory for the entire thing
  unsigned int length = Size();
  uint8_t *tmp = new uint8_t[length];

  unsigned int offset = 0;
  BlockVector::const_iterator iter = m_blocks.begin();
  for (; iter != m_blocks.end(); ++iter) {
    offset += (*iter)->Copy(tmp + offset, length - offset);
  }

  ola::FormatData(output, tmp, length);
  delete[] tmp;
}


/**
 * Append another block.
 */
void IOStack::PrependBlock() {
  MemoryBlock *block = m_pool->Allocate();
  if (!block) {
    OLA_FATAL << "Failed to allocate block, we're out of memory!";
  }
  block->SeekBack();  // put the block into prepend mode
  m_blocks.push_front(block);
}
}  // io
}  // ola
