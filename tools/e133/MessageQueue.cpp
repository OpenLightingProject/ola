/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * MessageQueue.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include "tools/e133/MessageQueue.h"

#include <ola/Callback.h>

// 1k is probably enough for userspace. The Linux kernel default is 4k,
// tunable via /proc/sys/net/core/wmem_{max,default}.
const unsigned int MessageQueue::DEFAULT_MAX_BUFFER_SIZE = 1024;

/**
 * Create a new MessageQueue.
 * @param descriptor the ConnectedDescriptor to send the data on
 * @param ss the SelectServer to use to register for on-write events.
 * @param memory_pool the pool to use for freeing MemoryBlocks
 * @param max_buffer_size the maximum amount of data to buffer. Note that
 *   because the underlying MemoryBlocks may be partially used, this does not
 *   reflect the actual amount of memory used (in pathological cases we may
 *   allocate up to max_buffer_size * memory_block_size bytes.
 */
MessageQueue::MessageQueue(ola::io::ConnectedDescriptor *descriptor,
                           ola::io::SelectServerInterface *ss,
                           ola::io::MemoryBlockPool *memory_pool,
                           unsigned int max_buffer_size)
  : m_descriptor(descriptor),
    m_ss(ss),
    m_output_buffer(memory_pool),
    m_associated(false),
    m_max_buffer_size(max_buffer_size) {
  m_descriptor->SetOnWritable(
      ola::NewCallback(this, &MessageQueue::PerformWrite));
}

/**
 * Cancel the callback.
 */
MessageQueue::~MessageQueue() {
  m_descriptor->SetOnWritable(NULL);
}


/**
 * Returns true if we've reached the specified maximum buffer size. No new
 * messages will be sent until the buffer drains.
 */
bool MessageQueue::LimitReached() const {
  return m_output_buffer.Size() >= m_max_buffer_size;
}


/**
 * Queue up the data in an IOStack to send on the underlying descriptor.
 * @param stack the IOStack to send. All data in this stack will be send and
 * the stack will be emptied.
 * @return true if the data was queued for sending, false if the internal
 *   buffer size has been exceeded.
 */
bool MessageQueue::SendMessage(ola::io::IOStack *stack) {
  if (LimitReached())
    return false;

  stack->MoveToIOQueue(&m_output_buffer);
  AssociateIfRequired();
  return true;
}


/**
 * Called when the descriptor is writeable, this does the actual write() call.
 */
void MessageQueue::PerformWrite() {
  m_descriptor->Send(&m_output_buffer);
  if (m_output_buffer.Empty() && m_associated)
    m_ss->RemoveWriteDescriptor(m_descriptor);
}


/**
 * Associate our descriptor with the SelectServer if we have data to send.
 */
void MessageQueue::AssociateIfRequired() {
  if (m_output_buffer.Empty())
    return;
  m_ss->AddWriteDescriptor(m_descriptor);
  m_associated = true;
}
