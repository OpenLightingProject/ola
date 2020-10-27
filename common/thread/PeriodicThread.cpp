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
 * PeriodicThread.cpp
 * A thread which executes a Callback periodically.
 * Copyright (C) 2015 Simon Newton
 */

#include "ola/thread/PeriodicThread.h"

#include <errno.h>
#include <string.h>

#include <ola/Logging.h>

namespace ola {
namespace thread {

PeriodicThread::PeriodicThread(const TimeInterval &delay,
                               PeriodicCallback *callback,
                               const Options &options)
    : Thread(options),
      m_delay(delay),
      m_callback(callback),
      m_terminate(false) {
  if (m_callback) {
    Start();
  }
}

void PeriodicThread::Stop() {
  {
    MutexLocker lock(&m_mutex);
    m_terminate = true;
  }
  m_condition.Signal();
  Join(NULL);
}

void *PeriodicThread::Run() {
  Clock clock;
  TimeStamp last_run_at;

  // Real time is used here because it gets passed into pthread_cond_timedwait
  // which expects an absolute time
  clock.CurrentRealTime(&last_run_at);
  if (!m_callback->Run()) {
    return NULL;
  }

  while (true) {
    {
      MutexLocker lock(&m_mutex);
      if (m_terminate) {
        return NULL;
      }
      if (m_condition.TimedWait(&m_mutex, last_run_at + m_delay)) {
        // Either m_terminate is true, or a spurious wake up
        if (m_terminate) {
          return NULL;
        }
        continue;
      }
    }
    // Real time is used here again to maintain clock consistency with the
    // previous call to CurrentRealTime()
    clock.CurrentRealTime(&last_run_at);
    if (!m_callback->Run()) {
      return NULL;
    }
  }
  return NULL;
}
}  // namespace thread
}  // namespace ola
