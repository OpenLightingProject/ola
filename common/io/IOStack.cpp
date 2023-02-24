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
using std::string;

/**
 * IOStack.
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
 * Read up to n bytes into the memory location data and shrink the IOQueue by
 * the amount read.
 */
unsigned int IOStack::Read(uint8_t *data, unsigned int length) {
  unsigned int bytes_read = 0;
  BlockVector::iterator iter = m_blocks.begin();
  while (iter != m_blocks.end() && bytes_read != length) {
    MemoryBlock *block = *iter;
    unsigned int bytes_copied = block->Copy(data + bytes_read,
                                            length - bytes_read);
    block->PopFront(bytes_copied);
    bytes_read += bytes_copied;
    if (block->Empty()) {
      m_pool->Release(block);
      iter = m_blocks.erase(iter);
    } else {
      iter++;
    }
  }
  return bytes_read;
}


/**
 * Read up to n bytes into the string output.
 */
unsigned int IOStack::Read(string *output, unsigned int length) {
  unsigned int bytes_remaining = length;
  BlockVector::iterator iter = m_blocks.begin();
  while (iter != m_blocks.end() && bytes_remaining) {
    MemoryBlock *block = *iter;
    unsigned int bytes_to_copy = std::min(block->Size(), bytes_remaining);
    output->append(reinterpret_cast<char*>(block->Data()), bytes_to_copy);
    bytes_remaining -= bytes_to_copy;
    if (block->Empty()) {
      m_pool->Release(block);
      iter = m_blocks.erase(iter);
    } else {
      iter++;
    }
  }
  return length - bytes_remaining;
}


/**
 * Return this IOStack as an array of IOVec structures.
 * Note: The IOVec array points at internal memory structures. This array is
 * invalidated when any non-const methods are called (Append, Pop etc.)
 *
 * Is the IOStack is empty, this will return NULL and set iocnt to 0.
 *
 * Use FreeIOVec() to release the IOVec array.
 */
const struct IOVec *IOStack::AsIOVec(int *iocnt) const {
  if (m_blocks.empty()) {
    *iocnt = 0;
    return NULL;
  }

  int max_number_of_blocks = m_blocks.size();
  int number_of_blocks = 0;

  struct IOVec *vector = new struct IOVec[max_number_of_blocks];
  struct IOVec *ptr = vector;
  BlockVector::const_iterator iter = m_blocks.begin();
  for (; iter != m_blocks.end(); ++iter, ++ptr, number_of_blocks++) {
    ptr->iov_base = (*iter)->Data();
    ptr->iov_len = (*iter)->Size();
  }

  *iocnt = number_of_blocks;
  return vector;
}


/**
 * Remove bytes from the stack
 */
void IOStack::Pop(unsigned int bytes_to_remove) {
  unsigned int bytes_removed = 0;
  BlockVector::iterator iter = m_blocks.begin();
  while (iter != m_blocks.end() && bytes_removed != bytes_to_remove) {
    MemoryBlock *block = *iter;
    bytes_removed += block->PopFront(bytes_to_remove - bytes_removed);
    if (block->Empty()) {
      m_pool->Release(block);
      iter = m_blocks.erase(iter);
    } else {
      iter++;
    }
  }
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


void IOStack::Purge() {
  m_pool->Purge();
}

/**
 * Dump this IOStack as a human readable string
 */
void IOStack::Dump(std::ostream *output) {
  unsigned int length = 0;
  BlockVector::const_iterator iter = m_blocks.begin();
  for (; iter != m_blocks.end(); ++iter) {
    length += (*iter)->Size();
  }

  // For now just alloc memory for the entire thing
  uint8_t *tmp = new uint8_t[length];

  unsigned int offset = 0;
  for (iter = m_blocks.begin(); iter != m_blocks.end(); ++iter) {
    offset += (*iter)->Copy(tmp + offset, length - offset);
  }

  ola::FormatData(output, tmp, offset);
  delete[] tmp;
}


/**
 * Append another block.
 */
void IOStack::PrependBlock() {
  MemoryBlock *block = m_pool->Allocate();
  if (!block) {
    OLA_FATAL << "Failed to allocate block, we're out of memory!";
  } else {
    block->SeekBack();  // put the block into prepend mode
    m_blocks.push_front(block);
  }
}
}  // namespace io
}  // namespace ola
