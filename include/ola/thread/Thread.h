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

namespace ola {
namespace thread {

typedef pthread_t ThreadId;

/**
 * A thread object to be subclassed.
 */
class Thread {
 public:
  explicit Thread(const std::string &name = "");
  virtual ~Thread() {}

  virtual bool Start();
  virtual bool FastStart();
  virtual bool Join(void *ptr = NULL);
  bool IsRunning();

  ThreadId Id() const { return m_thread_id; }

  std::string Name() const { return m_name; }

  // Called by pthread_create
  void *_InternalRun();

  static inline ThreadId Self() { return pthread_self(); }

 protected:
  // Sub classes implement this.
  virtual void *Run() = 0;

 private:
  pthread_t m_thread_id;
  bool m_running;
  const std::string m_name;
  Mutex m_mutex;  // protects m_running
  ConditionVariable m_condition;  // use to wait for the thread to start

  DISALLOW_COPY_AND_ASSIGN(Thread);
};
}  // namespace thread
}  // namespace ola
#endif  // INCLUDE_OLA_THREAD_THREAD_H_
