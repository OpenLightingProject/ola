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
 * Thread.cpp
 * A simple thread class
 * Copyright (C) 2010 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <pthread.h>

#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif

#include <string>

#include "ola/Logging.h"
#include "ola/thread/Thread.h"

namespace  {

/*
 * Called by the new thread
 */
void *StartThread(void *d) {
  ola::thread::Thread *thread = static_cast<ola::thread::Thread*>(d);
  return thread->_InternalRun();
}
}  // namespace

namespace ola {
namespace thread {

using std::string;

Thread::Thread(const string &name)
    : m_thread_id(),
      m_running(false),
      m_name(name) {
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
  int ret = pthread_create(&m_thread_id, NULL, StartThread,
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
  string truncated_name = m_name.substr(0, 15);

// There are 4 different variants of pthread_setname_np !
#ifdef HAVE_PTHREAD_SETNAME_NP_2
  pthread_setname_np(pthread_self(), truncated_name.c_str());
#endif

#ifdef HAVE_PTHREAD_SET_NAME_NP_2
  pthread_set_name_np(pthread_self(), truncated_name.c_str());
#endif

#ifdef HAVE_PTHREAD_SETNAME_NP_1
  pthread_setname_np(truncated_name.c_str());
#endif

#ifdef HAVE_PTHREAD_SETNAME_NP_3
  pthread_setname_np(pthread_self(), truncated_name.c_str(), NULL);
#endif
  {
    MutexLocker locker(&m_mutex);
    m_running = true;
  }
  m_condition.Signal();
  return Run();
}
}  // namespace thread
}  // namespace ola
