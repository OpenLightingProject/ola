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
 * ConsumerThread.h
 * An thread which consumes Callbacks from a queue and runs them.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_CONSUMERTHREAD_H_
#define INCLUDE_OLA_THREAD_CONSUMERTHREAD_H_

#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/thread/Thread.h>
#include <queue>

namespace ola {
namespace thread {

/**
 * A thread which waits on a queue, and when actions (callbacks) become
 * available, it pulls them from the queue and executes them.
 */
class ConsumerThread: public ola::thread::Thread {
 public :
  typedef BaseCallback0<void>* Action;
  /**
   * @param callback_queue the queue to pull actions from
   * @param shutdown a bool which is set to true if this thread is to finish.
   * @param mutex the Mutex object which protects the queue and shutdown
   *   variable.
   * @param condition_var the ConditionVariable to wait on. This signals when
   *   the queue is non-empty, or shutdown changes to true.
   * @param options The thread options.
   */
  ConsumerThread(std::queue<Action> *callback_queue,
                 const bool *shutdown,
                 Mutex *mutex,
                 ConditionVariable *condition_var,
                 const Thread::Options& options = Thread::Options())
      : Thread(options),
        m_callback_queue(callback_queue),
        m_shutdown(shutdown),
        m_mutex(mutex),
        m_condition_var(condition_var) {
  }
  ~ConsumerThread() {}
  void *Run();

 private:
  std::queue<Action> *m_callback_queue;
  const bool *m_shutdown;
  Mutex *m_mutex;
  ConditionVariable *m_condition_var;

  void EmptyQueue();

  DISALLOW_COPY_AND_ASSIGN(ConsumerThread);
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_CONSUMERTHREAD_H_
