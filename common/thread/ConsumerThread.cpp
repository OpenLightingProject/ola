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
 * ConsumerThread.h
 * An thread which consumes Callbacks from a queue and runs them.
 * Copyright (C) 2011 Simon Newton
 */

#include "ola/Logging.h"
#include "ola/thread/ConsumerThread.h"

namespace ola {
namespace thread {

/**
 * The run method, this loops, executing actions, until we're told to
 * terminate.
 */
void *ConsumerThread::Run() {
  m_mutex->Lock();
  while (true) {
    EmptyQueue();
    // Mutex is held here
    if (*m_shutdown) {
      m_mutex->Unlock();
      break;
    }

    m_condition_var->Wait(m_mutex);
  }
  return NULL;
}


/**
 * Drain the queue of actions
 * @pre We hold the Mutex
 * @post We hold the mutex and the queue is empty.
 */
void ConsumerThread::EmptyQueue() {
  while (true) {
    // we have the lock
    if (m_callback_queue->empty()) {
      break;
    }

    // action in queue
    Action action = m_callback_queue->front();
    m_callback_queue->pop();
    m_mutex->Unlock();

    action->Run();

    // reacquire the lock
    m_mutex->Lock();
  }
}
}  // thread
}  // ola
