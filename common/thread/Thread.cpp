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
#endif  // HAVE_CONFIG_H

#include <errno.h>
#ifdef _WIN32
// On MinGW, pthread.h pulls in Windows.h, which in turn pollutes the global
// namespace. We define VC_EXTRALEAN and WIN32_LEAN_AND_MEAN to reduce this.
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#endif  // _WIN32
#include <pthread.h>
#include <string.h>

#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif  // HAVE_PTHREAD_NP_H


#include <string>

#include "ola/Logging.h"
#include "ola/thread/Thread.h"
#include "ola/thread/Utils.h"

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

Thread::Options::Options(const std::string &name)
  : name(name),
    inheritsched(PTHREAD_EXPLICIT_SCHED) {
  // Default the scheduling options to the system-default values.
  pthread_attr_t attrs;
  pthread_attr_init(&attrs);
  struct sched_param param;
  pthread_attr_getschedpolicy(&attrs, &policy);
  pthread_attr_getschedparam(&attrs, &param);
  priority = param.sched_priority;
  pthread_attr_destroy(&attrs);
}

Thread::Thread(const Options &options)
    : m_thread_id(),
      m_running(false),
      m_options(options) {
  // Mac has a bug where PTHREAD_INHERIT_SCHED doesn't work. We work around
  // this by explicitly setting the policy and priority to match the current
  // thread if PTHREAD_INHERIT_SCHED was requested.
  if (m_options.inheritsched == PTHREAD_INHERIT_SCHED) {
    struct sched_param param;
    pthread_getschedparam(pthread_self(), &m_options.policy, &param);
    m_options.priority = param.sched_priority;
    m_options.inheritsched = PTHREAD_EXPLICIT_SCHED;
  }
}

bool Thread::Start() {
  MutexLocker locker(&m_mutex);
  if (m_running) {
    OLA_WARN << "Attempt to start already running thread " << Name();
    return false;
  }

  if (FastStart()) {
    m_condition.Wait(&m_mutex);
    return true;
  }
  return false;
}

bool Thread::FastStart() {
  pthread_attr_t attrs;
  pthread_attr_init(&attrs);

  if (m_options.inheritsched != PTHREAD_EXPLICIT_SCHED) {
    OLA_FATAL << "PTHREAD_EXPLICIT_SCHED not set, programming bug for "
              << Name() << "!";
    return false;
  }

  // glibc 2.8 and onwards has a bug where PTHREAD_EXPLICIT_SCHED won't be
  // honored unless the policy and priority are explicitly set. See
  // man pthread_attr_setinheritsched.
  // By fetching the default values in Thread::Options(), we avoid this bug.
  int ret = pthread_attr_setschedpolicy(&attrs, m_options.policy);
  if (ret) {
    OLA_WARN << "pthread_attr_setschedpolicy failed for " << Name()
             << ", policy " << m_options.policy << ": " << strerror(errno);
    pthread_attr_destroy(&attrs);
    return false;
  }

  struct sched_param param;
  param.sched_priority = m_options.priority;
  ret = pthread_attr_setschedparam(&attrs, &param);
  if (ret) {
    OLA_WARN << "pthread_attr_setschedparam failed for " << Name()
             << ", priority " << param.sched_priority << ": "
             << strerror(errno);
    pthread_attr_destroy(&attrs);
    return false;
  }

  ret = pthread_attr_setinheritsched(&attrs, PTHREAD_EXPLICIT_SCHED);
  if (ret) {
    OLA_WARN << "pthread_attr_setinheritsched to PTHREAD_EXPLICIT_SCHED "
             << "failed for " << Name() << ": " << strerror(errno);
    pthread_attr_destroy(&attrs);
    return false;
  }

  ret = pthread_create(&m_thread_id, &attrs, StartThread,
                       static_cast<void*>(this));

  pthread_attr_destroy(&attrs);

  if (ret) {
    OLA_WARN << "pthread create failed for " << Name() << ": "
             << strerror(ret);
    return false;
  }
  return true;
}

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

void *Thread::_InternalRun() {
  string truncated_name = m_options.name.substr(0, 15);

// There are 4 different variants of pthread_setname_np !
#ifdef HAVE_PTHREAD_SETNAME_NP_2
  pthread_setname_np(pthread_self(), truncated_name.c_str());
#endif  // HAVE_PTHREAD_SETNAME_NP_2

#if defined(HAVE_PTHREAD_SET_NAME_NP_2) || \
    defined(HAVE_PTHREAD_SET_NAME_NP_2_VOID)
  pthread_set_name_np(pthread_self(), truncated_name.c_str());
#endif  // defined(HAVE_PTHREAD_SET_NAME_NP_2) ||
// defined(HAVE_PTHREAD_SET_NAME_NP_2_VOID)

#ifdef HAVE_PTHREAD_SETNAME_NP_1
  pthread_setname_np(truncated_name.c_str());
#endif  // HAVE_PTHREAD_SETNAME_NP_1

#ifdef HAVE_PTHREAD_SETNAME_NP_3
  pthread_setname_np(pthread_self(), truncated_name.c_str(), NULL);
#endif  // HAVE_PTHREAD_SETNAME_NP_3

  int policy;
  struct sched_param param;
  pthread_getschedparam(pthread_self(), &policy, &param);

  OLA_INFO << "Thread " << Name() << ", policy " << PolicyToString(policy)
           << ", priority " << param.sched_priority;
  {
    MutexLocker locker(&m_mutex);
    m_running = true;
  }
  m_condition.Signal();
  return Run();
}
}  // namespace thread
}  // namespace ola
