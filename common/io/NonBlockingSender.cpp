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
 * NonBlockingSender.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include "ola/Callback.h"
#include "ola/io/IOQueue.h"
#include "ola/io/IOStack.h"
#include "ola/io/NonBlockingSender.h"

namespace ola {
namespace io {

const unsigned int NonBlockingSender::DEFAULT_MAX_BUFFER_SIZE = 1024;

NonBlockingSender::NonBlockingSender(ola::io::ConnectedDescriptor *descriptor,
                                     ola::io::SelectServerInterface *ss,
                                     ola::io::MemoryBlockPool *memory_pool,
                                     unsigned int max_buffer_size)
  : m_descriptor(descriptor),
    m_ss(ss),
    m_output_buffer(memory_pool),
    m_associated(false),
    m_max_buffer_size(max_buffer_size) {
  m_descriptor->SetOnWritable(
      ola::NewCallback(this, &NonBlockingSender::PerformWrite));
}

NonBlockingSender::~NonBlockingSender() {
  if (m_associated) {
    m_ss->RemoveWriteDescriptor(m_descriptor);
  }
  m_descriptor->SetOnWritable(NULL);
}

bool NonBlockingSender::LimitReached() const {
  return m_output_buffer.Size() >= m_max_buffer_size;
}

bool NonBlockingSender::SendMessage(ola::io::IOStack *stack) {
  if (LimitReached()) {
    return false;
  }

  stack->MoveToIOQueue(&m_output_buffer);
  AssociateIfRequired();
  return true;
}

bool NonBlockingSender::SendMessage(IOQueue *queue) {
  if (LimitReached()) {
    return false;
  }

  m_output_buffer.AppendMove(queue);
  AssociateIfRequired();
  return true;
}

/*
 * Called when the descriptor is writeable, this does the actual write() call.
 */
void NonBlockingSender::PerformWrite() {
  m_descriptor->Send(&m_output_buffer);
  if (m_output_buffer.Empty() && m_associated) {
    m_ss->RemoveWriteDescriptor(m_descriptor);
    m_associated = false;
  }
}

/*
 * Associate our descriptor with the SelectServer if we have data to send.
 */
void NonBlockingSender::AssociateIfRequired() {
  if (m_output_buffer.Empty()) {
    return;
  }
  m_ss->AddWriteDescriptor(m_descriptor);
  m_associated = true;
}
}  // namespace io
}  // namespace ola
