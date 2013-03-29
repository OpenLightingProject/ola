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
 * IOQueue.cpp
 * A non-contigous memory buffer
 * Copyright (C) 2012 Simon Newton
 */


#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/io/IOQueue.h>
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
 * IOQueue.
 * @param block_size the size of blocks to use.
 */
IOQueue::IOQueue()
    : m_pool(new MemoryBlockPool()),
      m_delete_pool(true) {
}


IOQueue::IOQueue(MemoryBlockPool *block_pool)
    : m_pool(block_pool),
      m_delete_pool(false) {
}

/**
 * Clean up
 */
IOQueue::~IOQueue() {
  Clear();

  if (m_delete_pool)
    delete m_pool;
}


/**
 * Return the amount of data in the buffer
 */
unsigned int IOQueue::Size() const {
  if (m_blocks.empty())
    return 0;

  unsigned int size = 0;
  BlockVector::const_iterator iter = m_blocks.begin();
  for (; iter != m_blocks.end(); ++iter)
    size += (*iter)->Size();
  return size;
}


/**
 * Append (length) bytes of data to the buffer
 */
void IOQueue::Write(const uint8_t *data, unsigned int length) {
  unsigned int bytes_written = 0;
  if (m_blocks.empty()) {
    AppendBlock();
  }

  while (true) {
    bytes_written += m_blocks.back()->Append(data + bytes_written,
                                             length - bytes_written);
    if (bytes_written == length) {
      return;
    }
    AppendBlock();
  }
}


/**
 * Read up to n bytes into the memory location data and shrink the IOQueue by
 * the amount read.
 */
unsigned int IOQueue::Read(uint8_t *data, unsigned int n) {
  unsigned int bytes_read = 0;
  BlockVector::iterator iter = m_blocks.begin();
  while (iter != m_blocks.end() && bytes_read != n) {
    MemoryBlock *block = *iter;
    unsigned int bytes_copied = block->Copy(data + bytes_read, n - bytes_read);
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
unsigned int IOQueue::Read(std::string *output, unsigned int n) {
  unsigned int bytes_remaining = n;
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
  return n - bytes_remaining;
}


/**
 * Copy the first n bytes into the region pointed to by data
 */
unsigned int IOQueue::Peek(uint8_t *data, unsigned int n) const {
  unsigned int bytes_read = 0;
  BlockVector::const_iterator iter = m_blocks.begin();
  while (iter != m_blocks.end() && bytes_read != n) {
    bytes_read += (*iter)->Copy(data + bytes_read, n - bytes_read);
    iter++;
  }
  return bytes_read;
}


/**
 * Remove the first n bytes from the buffer
 */
void IOQueue::Pop(unsigned int n) {
  unsigned int bytes_popped = 0;
  BlockVector::iterator iter = m_blocks.begin();
  while (iter != m_blocks.end() && bytes_popped != n) {
    MemoryBlock *block = *iter;
    bytes_popped += block->PopFront(n - bytes_popped);
    if (block->Empty()) {
      m_pool->Release(block);
      iter = m_blocks.erase(iter);
    } else {
      iter++;
    }
  }
}


/**
 * Return this IOQueue as an array of iovec structures.
 * Note: The iovec array points at internal memory structures. This array is
 * invalidated when any non-const methods are called (Append, Pop etc.)
 *
 * Is the IOQueue is empty, this will return NULL and set iocnt to 0.
 *
 * Use FreeIOVec() to release the iovec array.
 */
const struct iovec *IOQueue::AsIOVec(int *iocnt) const {
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
 * Append an MemoryBlock to this queue. This may leave a hole in the last block
 * before this method was called, but that's unavoidable without copying (which
 * we don't want to do).
 */
void IOQueue::AppendBlock(class MemoryBlock *block) {
  m_blocks.push_back(block);
}


/**
 * Remove all data from the IOQueue.
 */
void IOQueue::Clear() {
  BlockVector::iterator iter = m_blocks.begin();
  for (; iter != m_blocks.end(); ++iter)
    m_pool->Release(*iter);
  m_blocks.clear();
}

void IOQueue::Purge() {
  m_pool->Purge();
}

/**
 * Dump this IOQueue as a human readable string
 */
void IOQueue::Dump(std::ostream *output) {
  // For now just alloc memory for the entire thing
  unsigned int length = Size();
  uint8_t *tmp = new uint8_t[length];
  length = Peek(tmp, length);
  ola::FormatData(output, tmp, length);
  delete[] tmp;
}


/**
 * Append another block.
 */
void IOQueue::AppendBlock() {
  MemoryBlock *block = m_pool->Allocate();
  if (!block) {
    OLA_FATAL << "Failed to allocate block, we're out of memory!";
  }
  m_blocks.push_back(block);
}
}  // io
}  // ola
