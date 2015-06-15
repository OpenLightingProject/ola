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
 * ThreadPool.h
 * An executor which farms work out to a bunch of threads.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_THREADPOOL_H_
#define INCLUDE_OLA_THREAD_THREADPOOL_H_

#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/thread/ConsumerThread.h>
#include <ola/thread/Thread.h>
#include <queue>
#include <vector>

namespace ola {
namespace thread {

class ThreadPool {
 public :
  typedef ola::BaseCallback0<void>* Action;

  explicit ThreadPool(unsigned int thread_count)
      : m_thread_count(thread_count),
        m_shutdown(false) {
  }
  ~ThreadPool();
  bool Init();
  void JoinAll();
  void Execute(Action action);

 private:
  std::queue<Action> m_callback_queue;
  unsigned int m_thread_count;
  bool m_shutdown;
  Mutex m_mutex;
  ConditionVariable m_condition_var;
  std::vector<ConsumerThread*> m_threads;

  void JoinAllThreads();

  DISALLOW_COPY_AND_ASSIGN(ThreadPool);
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_THREADPOOL_H_
