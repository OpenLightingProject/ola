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
 * ExecutorThread.h
 * Run callbacks in a separate thread.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_EXECUTORTHREAD_H_
#define INCLUDE_OLA_THREAD_EXECUTORTHREAD_H_

#include <ola/Callback.h>
#include <ola/io/SelectServer.h>
#include <ola/thread/Thread.h>
#include <ola/thread/ConsumerThread.h>

#include <memory>
#include <queue>

namespace ola {
namespace thread {

/**
 * @brief Enables callbacks to be executed in a separate thread.
 * @param callback the callback to run.
 *
 * This can be used to deferred deletion of objects. e.g.
 *
 * ~~~~~~~~~~~~~~~~~~~~~

  ExecutorThread cleanup_thread;
  cleanup_thread.Start();
  Foo *f = new Foo();

  // Some time later ...
  // f will be deleted in a separate thread.
  cleanup_thread.Execute(DeletePointerCallback(f));

  cleanup_thread.Stop()
 * ~~~~~~~~~~~~~~~~~~~~~
 */
class ExecutorThread : public ola::thread::ExecutorInterface {
 public:
  /**
   * @brief Create a new ExecutorThread.
   * @param options The thread options to use
   */
  explicit ExecutorThread(const ola::thread::Thread::Options& options)
    : m_shutdown(false),
      m_thread(&m_callback_queue, &m_shutdown, &m_mutex, &m_condition_var,
               options) {
  }

  ~ExecutorThread();

  void Execute(ola::BaseCallback0<void> *callback);

  /**
   * @brief Block until all pending callbacks have been processed.
   *
   * The callbacks continue to be run in the ExecutorThread, so if the thread
   * has been stopped this will block indefinitely.
   */
  void DrainCallbacks();

  /**
   * @brief Start the ExecutorThread.
   * @returns true if the thread started, false if the thread was already
   *   running.
   *
   * Not thread safe, should only be called once.
   */
  bool Start();

  /**
   * @brief Stop this Executor thread.
   * @returns true if the Executor was stopped, false if it wasn't running.
   *
   * Stop() will halt the executor thread, and process any pending callbacks in
   * the current thread. Stop() will return once there is no pending
   * callbacks.
   *
   * Not thread safe, should only be called once.
   */
  bool Stop();

 private:
  // CallbackThread m_thread;
  std::queue<ola::BaseCallback0<void>*> m_callback_queue;
  bool m_shutdown;
  Mutex m_mutex;
  ConditionVariable m_condition_var;
  ConsumerThread m_thread;

  void RunRemaining();

  DISALLOW_COPY_AND_ASSIGN(ExecutorThread);
};

}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_EXECUTORTHREAD_H_
