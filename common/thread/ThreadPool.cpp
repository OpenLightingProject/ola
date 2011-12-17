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
 * ThreadPool.cpp
 * An executor which farms work out to a bunch of threads.
 * Copyright (C) 2011 Simon Newton
 */

#include "ola/Logging.h"
#include "ola/thread/Thread.h"
#include "ola/thread/ThreadPool.h"
#include "ola/thread/ConsumerThread.h"

namespace ola {
namespace thread {


/**
 * Clean up
 */
ThreadPool::~ThreadPool() {
  JoinAllThreads();
}


/**
 * Start the threads
 */
bool ThreadPool::Init() {
  if (!m_threads.empty()) {
    OLA_WARN << "Thread pool already started";
    return false;
  }

  for (unsigned int i = 1 ; i <= m_thread_count; i++) {
    ConsumerThread *thread = new ConsumerThread(
        &m_callback_queue,
        &m_shutdown,
        &m_mutex,
        &m_condition_var);
    if (!thread->Start()) {
      OLA_WARN << "Failed to start thread " << i <<
        ", aborting ThreadPool::Init()";
      JoinAllThreads();
      return false;
    }
    m_threads.push_back(thread);
  }
  return true;
}


/**
 * Join all threads
 */
void ThreadPool::JoinAll() {
  JoinAllThreads();
}


/**
 * Queue the callback.
 * Don't call this after JoinAll() otherwise the closure may not run and will
 * probably leak memory.
 */
void ThreadPool::Execute(ola::BaseCallback0<void> *closure) {
  MutexLocker locker(&m_mutex);
  if (m_shutdown) {
    OLA_WARN <<
      "Adding actions to a ThreadPool while it's shutting down, this "
      "will leak!";
  }
  m_callback_queue.push(closure);
  m_condition_var.Signal();
}


/**
 * Join all threads.
 */
void ThreadPool::JoinAllThreads() {
  if (m_threads.empty())
    return;

  {
    MutexLocker locker(&m_mutex);
    m_shutdown = true;
    m_condition_var.Broadcast();
  }

  while (!m_threads.empty()) {
    ConsumerThread *thread = m_threads.back();
    m_threads.pop_back();
    thread->Join();
    delete thread;
  }
}
}  // thread
}  // ola
