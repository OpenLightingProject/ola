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
 * Thread.cpp
 * A simple thread class
 * Copyright (C) 2010 Simon Newton
 */

#include <pthread.h>
#include "ola/Logging.h"
#include "ola/thread/Thread.h"

namespace ola {
namespace thread {

/*
 * Called by the new thread
 */
void *StartThread(void *d) {
  Thread *thread = static_cast<Thread*>(d);
  return thread->_InternalRun();
}


/*
 * Start this thread. This only returns only the thread is running.
 */
bool Thread::Start() {
  MutexLocker locker(&m_mutex);
  if (m_running) {
    OLA_WARN << "Attempt to start already running thread";
    return false;
  }

  if (FastStart()) {
    m_condition.Wait(&m_mutex);
    return true;
  }
  return false;
}


/**
 * This launches a new thread and returns immediately. Don't use this unless
 * you know what you're doing as it introduces a race condition with Join()
 */
bool Thread::FastStart() {
  int ret = pthread_create(&m_thread_id,
                           NULL,
                           ola::thread::StartThread,
                           static_cast<void*>(this));
  if (ret) {
    OLA_WARN << "pthread create failed";
    return false;
  }
  return true;
}


/*
 * Join this thread
 * @returns false if the thread wasn't running or didn't stop, true otherwise.
 */
bool Thread::Join(void *ptr) {
  {
    MutexLocker locker(&m_mutex);
    if (!m_running)
      return false;
  }
  int ret = pthread_join(m_thread_id, &ptr);
  m_running = false;
  return 0 == ret;
}

bool Thread::IsRunning() {
  MutexLocker locker(&m_mutex);
  return m_running;
}


/**
 * Mark the thread as running and call the main Run method
 */
void *Thread::_InternalRun() {
  {
    MutexLocker locker(&m_mutex);
    m_running = true;
  }
  m_condition.Signal();
  return Run();
}
}  // thread
}  // ola
