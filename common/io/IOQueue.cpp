/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * IOQueue.cpp
 * A non-contigous memory buffer
 * Copyright (C) 2012 Simon Newton
 */


#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/io/IOQueue.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <deque>
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
IOQueue::IOQueue(unsigned int block_size)
    : m_block_size(block_size),
      m_first(NULL),
      m_last(NULL) {
}


/**
 * Clean up
 */
IOQueue::~IOQueue() {
  Purge();

  BlockVector::iterator iter = m_blocks.begin();
  for (; iter != m_blocks.end(); ++iter)
    delete[] *iter;

  m_blocks.clear();
}


/**
 * Return the amount of data in the buffer
 */
unsigned int IOQueue::Size() const {
  if (m_blocks.empty())
    return 0;

  return (m_blocks.size() * m_block_size -
          FreeSpaceInLastBlock() -
          FreeSpaceInFirstBlock());
}


/**
 * Append (length) bytes of data to the buffer
 */
void IOQueue::Write(const uint8_t *data, unsigned int length) {
  unsigned int offset = 0;

  // use up any remaining space in this block
  unsigned int free_space = FreeSpaceInLastBlock();
  if (free_space > 0) {
    unsigned int data_to_copy = min(free_space, length);
    memcpy(m_last, data, data_to_copy);
    m_last += data_to_copy;
    offset += data_to_copy;
  }

  // add new blocks as needed
  while (offset != length) {
    AppendBlock();
    unsigned int data_to_copy = std::min(m_block_size, length - offset);
    memcpy(m_last, data + offset, data_to_copy);
    m_last += data_to_copy;
    offset += data_to_copy;
  }
}


/**
 * Read up to n bytes into the memory location data.
 */
unsigned int IOQueue::Read(uint8_t *data, unsigned int n) {
  unsigned int size = Peek(data, n);
  Pop(size);
  return size;
}


/**
 * Read up to n bytes into the memory location data.
 */
unsigned int IOQueue::Read(std::string *output, unsigned int n) {
  unsigned int offset = 0;
  unsigned int size_of_first = SizeOfFirstBlock();
  unsigned int amount_to_copy = min(n, size_of_first);

  // copy as much as we need from the first block
  output->append(reinterpret_cast<char*>(m_first), amount_to_copy);
  if (n <= size_of_first)
    return n;

  offset += amount_to_copy;

  // now copy from the remaining blocks
  BlockVector::const_iterator iter = m_blocks.begin();

  while (offset < n) {
    iter++;
    amount_to_copy = min(n - offset, m_block_size);
    output->append(reinterpret_cast<char*>(*iter), amount_to_copy);
    offset += amount_to_copy;
  }
  return offset;
}


/**
 * Copy the first n bytes into the region pointed to by data
 */
unsigned int IOQueue::Peek(uint8_t *data, unsigned int n) const {
  if (n > Size()) {
    OLA_WARN << "Attempt to peek " << n << " bytes, size is only " << Size();
    n = Size();
  }

  unsigned int offset = 0;
  unsigned int size_of_first = SizeOfFirstBlock();
  unsigned int amount_to_copy = min(n, size_of_first);

  // copy as much as we need from the first block
  memcpy(data, m_first, amount_to_copy);
  if (n <= size_of_first)
    return n;

  offset += amount_to_copy;

  // now copy from the remaining blocks
  BlockVector::const_iterator iter = m_blocks.begin();

  while (offset < n) {
    iter++;
    amount_to_copy = min(n - offset, m_block_size);
    memcpy(data + offset, *iter, amount_to_copy);
    offset += amount_to_copy;
  }
  return offset;
}


/**
 * Remove the first n bytes from the buffer
 */
void IOQueue::Pop(unsigned int n) {
  if (n > Size()) {
    OLA_WARN << "Attempt to pop " << n << " bytes, size is only " << Size();
    n = Size();
  }

  unsigned int offset = 0;
  while (offset < n) {
    unsigned int size_of_first = SizeOfFirstBlock();
    unsigned int amount_to_remove = n - offset;
    if (amount_to_remove >= size_of_first) {
      // entire block
      PopBlock();
      offset += size_of_first;
    } else {
      m_first += amount_to_remove;
      offset += amount_to_remove;
    }
  }
}


/**
 * Return this IOQueue as an array of iovec structures.
 * Note: The iovec array points at internal memory structures. This array is
 * invalidated when any non-const methods are called (Append, Pop etc.)
 *
 * Free the iovec array with FreeIOVec()
 */
const struct iovec *IOQueue::AsIOVec(int *iocnt) {
  *iocnt = m_blocks.size();
  struct iovec *vector = new struct iovec[*iocnt];

  // the first block
  vector[0].iov_base = m_first;
  vector[0].iov_len = SizeOfFirstBlock();

  // remaining blocks
  BlockVector::const_iterator iter = m_blocks.begin();
  iter++;

  struct iovec *ptr = vector + 1;
  while (iter != m_blocks.end()) {
    ptr->iov_base = *iter;
    if (iter == m_last_block)
      ptr->iov_len = SizeOfLastLBlock();
    else
      ptr->iov_len = m_block_size;
    ptr++;
    iter++;
  }
  return vector;
}


/**
 * Free a iovec structure
 */
void IOQueue::FreeIOVec(const struct iovec *iov) {
  delete[] iov;
}


/**
 * Append an iov to this IOQueue
 */
void IOQueue::AppendIOVec(const struct iovec *iov, int iocnt) {
  for (int i = 0; i < iocnt; iov++, i++)
    Write(reinterpret_cast<const uint8_t*>(iov->iov_base), iov->iov_len);
}


/**
 * Purge any free blocks
 */
void IOQueue::Purge() {
  while (!m_free_blocks.empty()) {
    uint8_t *block = m_free_blocks.front();
    delete[] block;
    m_free_blocks.pop();
  }
}


/**
 * Dump this IOQueue as a human readable string
 */
void IOQueue::Dump(std::ostream *output) {
  // for now just alloc memory for the entire thing
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
  uint8_t *block = NULL;
  if (m_free_blocks.empty()) {
    block = new uint8_t[m_block_size];
    OLA_DEBUG << "new block allocated at @" << reinterpret_cast<int*>(block);
  } else {
    block = m_free_blocks.front();
    m_free_blocks.pop();
  }

  if (m_blocks.empty()) {
    m_first = block;
    m_blocks.push_back(block);
    m_last_block = m_blocks.begin();
  } else {
    m_blocks.push_back(block);
    m_last_block++;
  }

  m_last = block;
}


/**
 * Remove the first block
 * @pre m_blocks is not empty
 */
void IOQueue::PopBlock() {
  uint8_t *free_block = m_blocks.front();
  m_free_blocks.push(free_block);
  m_blocks.pop_front();
  if (m_blocks.empty()) {
    m_first = NULL;
    m_last = NULL;
  } else {
    m_first = *(m_blocks.begin());
  }
}
}  // io
}  // ola
