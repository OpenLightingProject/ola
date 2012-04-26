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
 * IOQueue.h
 * A non-contigous memory buffer that operates as a queue.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_IOQUEUE_H_
#define INCLUDE_OLA_IO_IOQUEUE_H_

#include <stdint.h>
#include <sys/uio.h>
#include <deque>
#include <iostream>
#include <queue>

namespace ola {
namespace io {


/**
 * IOQueue.
 */
class IOQueue {
  public:
    explicit IOQueue(unsigned int block_size = DEFAULT_BLOCK_SIZE);
    ~IOQueue();

    unsigned int Size() const;
    bool Empty() const {
      return m_blocks.empty();
    }

    void Append(const uint8_t *data, unsigned int length);
    unsigned int Peek(uint8_t *data, unsigned int length) const;
    void Pop(unsigned int n);

    const struct iovec *AsIOVec(int *iocnt);
    void FreeIOVec(const struct iovec *iov);
    void AppendIOVec(const struct iovec *iov, int iocnt);

    void Purge();

    void Dump(std::ostream *output);

  private:
    typedef std::deque<uint8_t*> BlockVector;

    BlockVector m_blocks;
    std::queue<uint8_t*> m_free_blocks;
    BlockVector::iterator m_last_block;
    const unsigned int m_block_size;
    // last points to one more than the last element
    uint8_t *m_first, *m_last;

    inline unsigned int SizeOfFirstBlock() const {
      if (m_blocks.empty())
        return 0;

      if (m_last_block == m_blocks.begin()) {
        // first == last
        return m_last - m_first;
      } else {
        return m_block_size - FreeSpaceInFirstBlock();
      }
    }

    /**
     * The amount of free space at the start of the first block
     */
    inline unsigned int FreeSpaceInFirstBlock() const {
      if (m_blocks.empty())
        return 0;

      return m_first - *(m_blocks.begin());
    }

    inline unsigned int SizeOfLastLBlock() const {
      if (m_blocks.empty())
        return 0;

      if (m_last_block == m_blocks.begin()) {
        // first == last
        return m_last - m_first;
      } else {
        return m_last - *m_last_block;
      }
    }

    /**
     * The amount of free space at the end of the last block
     */
    inline unsigned int FreeSpaceInLastBlock() const {
      if (m_blocks.empty())
        return 0;
      return *m_last_block + m_block_size - m_last;
    }

    void AppendBlock();
    void PopBlock();

    // no copying / assignment for now
    IOQueue(const IOQueue&);
    IOQueue& operator=(const IOQueue&);

    // default to 1k blocks
    static const unsigned int DEFAULT_BLOCK_SIZE = 1024;
};
}  // io
}  // ola
#endif  // INCLUDE_OLA_IO_IOQUEUE_H_
