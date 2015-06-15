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
 * Thread.h
 * A thread object.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_THREAD_THREAD_H_
#define INCLUDE_OLA_THREAD_THREAD_H_

#ifdef _WIN32
// On MinGW, pthread.h pulls in Windows.h, which in turn pollutes the global
// namespace. We define VC_EXTRALEAN and WIN32_LEAN_AND_MEAN to reduce this.
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <pthread.h>
#include <ola/base/Macro.h>
#include <ola/thread/Mutex.h>

#include <string>

#if defined(_WIN32) && defined(__GNUC__)
inline std::ostream& operator<<(std::ostream &stream,
                                 const ptw32_handle_t &handle) {
  stream << handle.p;
  return stream;
}
#endif

namespace ola {
namespace thread {

typedef pthread_t ThreadId;

/**
 * A thread object that can be subclassed.
 */
class Thread {
 public:
  /**
   * @brief Thread options.
   *
   * The Scheduler options default to PTHREAD_EXPLICIT_SCHED, with the policy
   * and priority set to the default values for the platform.
   */
  struct Options {
   public:
    /**
     * @brief The name of the thread.
     */
    std::string name;

    /**
     * @brief The scheduling policy.
     *
     * Should be one of SCHED_OTHER (usually the platform default), SCHED_FIFO
     *   or SCHED_RR.
     */
    int policy;

    /**
     * @brief The thread priority.
     *
     * Defaults to the default value of the platform.
     */
    int priority;

    /**
     * @brief The scheduling mode, either PTHREAD_EXPLICIT_SCHED or
     *   PTHREAD_INHERIT_SCHED.
     *
     * Defaults to PTHREAD_EXPLICIT_SCHED.
     */
    int inheritsched;

    /**
     * @brief Create new thread Options.
     * @param name the name of the thread.
     */
    explicit Options(const std::string &name = "");
  };

  /**
   * @brief Create a new thread with the specified thread options.
   * @param options the thread's options
   */
  explicit Thread(const Options &options = Options());

  /**
   * @brief Destructor
   */
  virtual ~Thread() {}

  /**
   * @brief Start the thread and wait for the thread to be running.
   * @returns true if the thread started, false otherwise.
   *
   * This will block until the thread is running. Use FastStart() if you don't
   * want to block.
   */
  virtual bool Start();

  /**
   * @brief Start the thread and return immediately.
   * @returns true if the thread started, false otherwise.
   *
   * Don't use this unless you know what you're doing, since it introduces a
   * race condition with Join().
   */
  virtual bool FastStart();

  /**
   * @brief Join this thread.
   * @param[out] ptr The value returned from the thread.
   * @returns false if the thread wasn't running or didn't stop, true otherwise.
   */
  virtual bool Join(void *ptr = NULL);

  /**
   * @brief Check if the thread is running.
   * @return true if the thread was running at some point in the past.
   *
   * This is best-effort only, since the thread may stop after IsRunning()
   * returns.
   */
  bool IsRunning();

  /**
   * @brief Return the thread id.
   * @returns the thread id.
   */
  ThreadId Id() const { return m_thread_id; }

  /**
   * @brief Return the thread name.
   * @returns the thread name.
   *
   * This may differ from the name assigned with pthread_setname, since the
   * latter has a limit of 16 characters.
   */
  std::string Name() const { return m_options.name; }

  /**
   * @private
   * Called by pthread_create. Internally this calls Run().
   */
  void* _InternalRun();

  /**
   * @brief Returns the current thread's id.
   * @returns the current thread's id.
   */
  static inline ThreadId Self() { return pthread_self(); }

 protected:
  /**
   * @brief The entry point for the new thread.
   * @returns A value returned to Join().
   *
   * Sub classes must implement this.
   */
  virtual void *Run() = 0;

 private:
  pthread_t m_thread_id;
  bool m_running;
  Options m_options;
  Mutex m_mutex;  // protects m_running
  ConditionVariable m_condition;  // use to wait for the thread to start

  DISALLOW_COPY_AND_ASSIGN(Thread);
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_THREAD_H_
