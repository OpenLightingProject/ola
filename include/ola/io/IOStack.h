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
 * IOStack.h
 * A non-contigous memory buffer that operates as a stack (LIFO).
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_IOSTACK_H_
#define INCLUDE_OLA_IO_IOSTACK_H_

#include <ola/io/IOVecInterface.h>
#include <ola/io/InputBuffer.h>
#include <ola/io/OutputBuffer.h>
#include <stdint.h>
#include <sys/uio.h>
#include <deque>
#include <iostream>
#include <queue>
#include <string>

namespace ola {
namespace io {

/**
 * IOStack.
 */
class IOStack: public IOVecInterface,
               public InputBufferInterface,
               public OutputBufferInterface {
 public:
    IOStack();
    explicit IOStack(class MemoryBlockPool *block_pool);

    ~IOStack();

    unsigned int Size() const;

    bool Empty() const {
      // guard against the case of Empty blocks.
      return m_blocks.empty() || Size() == 0;
    }

    // From OutputBufferInterface
    void Write(const uint8_t *data, unsigned int length);

    // From InputBufferInterface, these reads consume data from the buffer.
    unsigned int Read(uint8_t *data, unsigned int length);
    unsigned int Read(std::string *output, unsigned int length);

    // From IOVecInterface
    const struct iovec *AsIOVec(int *io_count) const;
    void Pop(unsigned int n);

    // 0-copy append to an IOQueue
    void MoveToIOQueue(class IOQueue *queue);

    // purge the underlying memory pool
    void Purge();

    void Dump(std::ostream *output);

 private:
    typedef std::deque<class MemoryBlock*> BlockVector;

    class MemoryBlockPool* m_pool;
    bool m_delete_pool;

    BlockVector m_blocks;

    void PrependBlock();

    // no copying / assignment for now
    IOStack(const IOStack&);
    IOStack& operator=(const IOStack&);
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_IOSTACK_H_
