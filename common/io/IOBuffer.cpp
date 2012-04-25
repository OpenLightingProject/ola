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
 * IOBuffer.cpp
 * A non-contigous memory buffer
 * Copyright (C) 2012 Simon Newton
 */


#include <ola/Logging.h>
#include <ola/io/IOBuffer.h>
#include <stdint.h>
#include <algorithm>
#include <deque>
#include <queue>

namespace ola {
namespace io {

using std::min;


/**
 * IOBuffer.
 * @param block_size the size of blocks to use.
 */
IOBuffer::IOBuffer(unsigned int block_size)
    : m_block_size(block_size),
      m_first(NULL),
      m_last(NULL) {
}


/**
 * Clean up
 */
IOBuffer::~IOBuffer() {
  Purge();

  BlockVector::iterator iter = m_blocks.begin();
  for (; iter != m_blocks.end(); ++iter)
    delete[] *iter;

  m_blocks.clear();
}


/**
 * Return the amount of data in the buffer
 */
unsigned int IOBuffer::Size() const {
  if (m_blocks.empty())
    return 0;

  return (m_blocks.size() * m_block_size -
          FreeSpaceInLastBlock() -
          FreeSpaceInFirstBlock());
}


/**
 * Append (length) bytes of data to the buffer
 */
void IOBuffer::Append(const uint8_t *data, unsigned int length) {
  unsigned int offset = 0;

  OLA_INFO << "free space in block in " << FreeSpaceInLastBlock();
  // use up any remaining space in this block
  unsigned int free_space = FreeSpaceInLastBlock();
  if (free_space > 0) {
    unsigned int data_to_copy = min(free_space, length);
    OLA_INFO << "filling " << data_to_copy << " in last block";
    memcpy(m_last, data, data_to_copy);
    m_last += data_to_copy;
    offset += data_to_copy;
  }
  OLA_INFO << "out of blocks, offset is " << offset;

  // add new blocks as needed
  while (offset != length) {
    AppendBlock();
    unsigned int data_to_copy = std::min(m_block_size, length - offset);
    memcpy(m_last, data + offset, data_to_copy);
    OLA_INFO << "copying " << data_to_copy << " bytes";
    m_last += data_to_copy;
    offset += data_to_copy;
  }

  /*
  OLA_INFO << FreeSpaceInFirstBlock();
  OLA_INFO << SizeOfFirstBlock();
  OLA_INFO << FreeSpaceInLastBlock();
  OLA_INFO << SizeOfLastLBlock();
  */
}


/**
 * Copy the first n bytes into the region pointed to by data
 */
unsigned int IOBuffer::Peek(uint8_t *data, unsigned int n) const {
  if (n > Size()) {
    OLA_WARN << "Attempt to peek " << n << " bytes, size is only " << Size();
    n = Size();
  }

  unsigned int offset = 0;
  unsigned int size_of_first = SizeOfFirstBlock();
  unsigned int amount_to_copy = min(n, size_of_first);

  // copy as much as we need from the first block
  memcpy(data, m_first, amount_to_copy);
  OLA_INFO << "copied " << amount_to_copy << " from the first block of size "
    << size_of_first;
  if (n <= size_of_first)
    return n;

  offset += amount_to_copy;

  // now copy from the remaining blocks
  BlockVector::const_iterator iter = m_blocks.begin();

  while (offset < n) {
    iter++;
    amount_to_copy = n - offset;
    OLA_INFO << "amount to copy is " << amount_to_copy;
    if (amount_to_copy > m_block_size) {
      // entire block
      memcpy(data + offset, *iter, m_block_size);
      offset += m_block_size;
    } else {
      memcpy(data + offset, *iter, amount_to_copy);
      offset += amount_to_copy;
    }
  }

  return offset;
}


/**
 * Remove the first n bytes from the buffer
 */
void IOBuffer::Pop(unsigned int n) {
  if (n > Size()) {
    OLA_WARN << "Attempt to pop " << n << " bytes, size is only " << Size();
    n = Size();
  }

  unsigned int offset = 0;
  while (offset < n) {
    unsigned int size_of_first = SizeOfFirstBlock();
    unsigned int amount_to_remove = n - offset;
    OLA_INFO << "amount to remove is " << amount_to_remove <<
      ", size of first block is " << size_of_first;
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
 * Return this IOBuffer as an array of iovec structures.
 * Note: The iovec array points at internal memory structures. This array is
 * invalidated when any non-const methods are called (Append, Pop etc.)
 *
 * Free the iovec array with FreeIOVec()
 */
const struct iovec *IOBuffer::AsIOVec(int *iocnt) {
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
void IOBuffer::FreeIOVec(const struct iovec *iov) {
  delete[] iov;
}


/**
 * Append an iov to this IOBuffer
 */
void IOBuffer::AppendIOVec(const struct iovec *iov, int iocnt) {
  for (int i = 0; i < iocnt; iov++, i++)
    Append(reinterpret_cast<const uint8_t*>(iov->iov_base), iov->iov_len);
}


/**
 * Purge any free blocks
 */
void IOBuffer::Purge() {
  while (!m_free_blocks.empty()) {
    uint8_t *block = m_free_blocks.front();
    delete[] block;
    m_free_blocks.pop();
  }
}


/**
 * Append another block.
 */
void IOBuffer::AppendBlock() {
  uint8_t *block = NULL;
  if (m_free_blocks.empty()) {
    block = new uint8_t[m_block_size];
    OLA_INFO << "new block alloced at @" << (int*) block;
  } else {
    OLA_INFO << "recycle old block";
    block = m_free_blocks.front();
    m_free_blocks.pop();
  }

  OLA_INFO << "appending block @ " << (int*) block;

  if (m_blocks.empty()) {
    m_first = block;
    m_blocks.push_back(block);
    m_last_block = m_blocks.begin();
  } else {
    m_blocks.push_back(block);
    m_last_block++;
  }

  m_last = block;
  OLA_INFO << "last points to @" << (int*) m_last;
}


/**
 * Remove the first block
 * @pre m_blocks is not empty
 */
void IOBuffer::PopBlock() {
  uint8_t *free_block = m_blocks.front();
  m_free_blocks.push(free_block);
  m_blocks.pop_front();
  BlockVector::iterator iter = m_blocks.begin();
  m_first = *(m_blocks.begin());
}
}  // io
}  // ola
