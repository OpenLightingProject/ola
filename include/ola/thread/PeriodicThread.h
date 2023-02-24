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
 * PeriodicThread.h
 * A thread which executes a Callback periodically.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_PERIODICTHREAD_H_
#define INCLUDE_OLA_THREAD_PERIODICTHREAD_H_

#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/thread/Thread.h>
#include <ola/Clock.h>

#include <string>

namespace ola {
namespace thread {

/**
 * @brief A thread which executes a Callback.
 */
class PeriodicThread : private Thread {
 public:
  /**
   * @brief The callback to run.
   *
   * If false is returned, the thread will stop.
   */
  typedef Callback0<bool> PeriodicCallback;

  /**
   * Create a new PeriodicThread.
   * @param delay The delay between invoking the callback.
   * @param callback the callback to run in the new thread.
   * @param options the thread's options.
   *
   * The thread will start running and immediately run the callback. This may
   * happen before the constructor returns.
   */
  explicit PeriodicThread(const TimeInterval &delay,
                          PeriodicCallback *callback,
                          const Options &options = Options());

  /**
   * @brief Stop the PeriodicThread.
   *
   * This blocks until the thread is no longer running.
   */
  void Stop();

 protected:
  void *Run();

 private:
  TimeInterval m_delay;
  PeriodicCallback *m_callback;

  bool m_terminate;
  ola::thread::Mutex m_mutex;
  ola::thread::ConditionVariable m_condition;

  DISALLOW_COPY_AND_ASSIGN(PeriodicThread);
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_PERIODICTHREAD_H_
