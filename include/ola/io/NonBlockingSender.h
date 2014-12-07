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
 * NonBlockingSender.h
 * Copyright (C) 2013 Simon Newton
 *
 * Explaination:
 *  If we just write IOStacks directly to ConnectedDescriptors, we may not be
 *  able to write the entire message. This can happen if the remote end is slow
 *  to ack and data builds up in the kernel socket buffer.
 *
 *  This class abstracts the caller from having to deal with this situation. At
 *  construction time we specify the maximum number of message bytes we want to
 *  buffer. Once the buffer reaches this size subsequent calls to SendMessage
 *  will return false.
 */

#ifndef INCLUDE_OLA_IO_NONBLOCKINGSENDER_H_
#define INCLUDE_OLA_IO_NONBLOCKINGSENDER_H_

#include <ola/io/Descriptor.h>
#include <ola/io/IOQueue.h>
#include <ola/io/MemoryBlockPool.h>
#include <ola/io/OutputBuffer.h>
#include <ola/io/SelectServerInterface.h>

namespace ola {
namespace io {

/**
 * @brief Write data to ConnectedDescriptors without blocking or losing data
 *
 * A NonBlockingSender handles writing data from IOStacks or IOQueues to a
 * ConnectedDescriptor. On calling SendMessage() the data from the stack or
 * queue is 0-copied to an internal buffer and then as much as possible is
 * written to the ConnectedDescriptor using scatter/gather I/O calls (if
 * available). If there is more data than fits in the descriptor's socket
 * buffer, the remaining data is held in the internal buffer.
 *
 * The internal buffer has a limit on the size. Once the limit is
 * exceeded, calls to SendMessage() will return false. The limit is a soft
 * limit however, a call to SendMessage() may cause the buffer to exceed the
 * internal limit, provided the limit has not already been reached.
 */
class NonBlockingSender {
 public:
  /**
   * @brief Create a new NonBlockingSender.
   * @param descriptor the ConnectedDescriptor to send on, ownership is not
   *   transferred.
   * @param ss the SelectServer to use to register for on-write events.
   * @param memory_pool the pool to return MemoryBlocks to
   * @param max_buffer_size the maximum amount of data to buffer. Note that
   *   because the underlying MemoryBlocks may be partially used, this does not
   *   reflect the actual amount of memory used (in pathological cases we may
   *   allocate up to max_buffer_size * memory_block_size bytes.
   */
  NonBlockingSender(ola::io::ConnectedDescriptor *descriptor,
                    ola::io::SelectServerInterface *ss,
                    ola::io::MemoryBlockPool *memory_pool,
                    unsigned int max_buffer_size = DEFAULT_MAX_BUFFER_SIZE);

  /**
   * @brief Destructor
   */
  ~NonBlockingSender();

  /**
   * @brief Check if the limit for the internal buffer has been reached.
   * @return true if the limit has been reached, false otherwise.
   */
  bool LimitReached() const;

  /**
   * @brief Send the contents of an IOStack on the ConnectedDescriptor.
   * @param stack the IOStack to send. All data in this stack will be sent and
   *   the stack will be emptied.
   * @returns true if the contents of the stack were buffered for transmit,
   *   false the limit for this NonBlockingSender had already been reached.
   */
  bool SendMessage(class IOStack *stack);

  /**
   * @brief Send the contents of an IOQueue on the ConnectedDescriptor.
   * @param queue the IOQueue to send. All data in this queue will be sent and
   *   the queue will be emptied.
   * @returns true if the contents of the stack were buffered for transmit,
   *   false the limit for this NonBlockingSender had already been reached.
   */
  bool SendMessage(IOQueue *queue);

  /**
   * @brief The default max internal buffer size.
   *
   * Once this limit has been reached, calls to SendMessage() will return
   * false.
   *
   * 1k is probably enough for userspace. The Linux kernel default is 4k,
   * tunable via /proc/sys/net/core/wmem_{max,default}.
   */
  static const unsigned int DEFAULT_MAX_BUFFER_SIZE;

 private:
  ola::io::ConnectedDescriptor *m_descriptor;
  ola::io::SelectServerInterface *m_ss;
  ola::io::IOQueue m_output_buffer;
  bool m_associated;
  unsigned int m_max_buffer_size;

  void PerformWrite();
  void AssociateIfRequired();

  DISALLOW_COPY_AND_ASSIGN(NonBlockingSender);
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_NONBLOCKINGSENDER_H_
