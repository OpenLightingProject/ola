/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * KarateThread.cpp
 * Thread for the karate device
 * Copyright (C) 2005 Simon Newton
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>
#include <string>

#include "ola/Clock.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "plugins/karate/KarateLight.h"
#include "plugins/karate/KarateThread.h"

namespace ola {
namespace plugin {
namespace karate {

using ola::thread::Mutex;
using ola::thread::MutexLocker;
using std::string;

/**
 * @brief Create a new KarateThread object
 */
KarateThread::KarateThread(const string &path)
    : ola::thread::Thread(),
      m_path(path),
      m_term(false) {
}


/**
 * @brief Run this thread
 */
void *KarateThread::Run() {
  bool write_success;
  Clock clock;

  KarateLight k(m_path);
  k.Init();

  while (true) {
    {
      MutexLocker lock(&m_term_mutex);
      if (m_term)
        break;
    }

    if (!k.IsActive()) {
      // try to reopen the device...
      TimeStamp wake_up;
      // Use real time here because wake_up is passed to pthread_cond_timedwait
      clock.CurrentRealTime(&wake_up);
      wake_up += TimeInterval(2, 0);

      // wait for either a signal that we should terminate, or ts seconds
      m_term_mutex.Lock();
      if (m_term)
        break;
      m_term_cond.TimedWait(&m_term_mutex, wake_up);
      m_term_mutex.Unlock();

      OLA_WARN << "Re-Initialising device " << m_path;
      k.Init();

    } else {
      {
        MutexLocker locker(&m_mutex);
        write_success = k.SetColors(m_buffer);
      }
      if (!write_success) {
        OLA_WARN << "Failed to write color data";
      }  else {
        usleep(20000);  // 50Hz
      }
    }  // port is okay
  }
  return NULL;
}


/**
 * @brief Stop the thread
 */
bool KarateThread::Stop() {
  {
    MutexLocker locker(&m_mutex);
    m_term = true;
  }
  m_term_cond.Signal();
  return Join();
}


/**
 * @brief Store the data in the shared buffer.
 */
bool KarateThread::WriteDmx(const DmxBuffer &buffer) {
  MutexLocker locker(&m_mutex);
  // avoid the reference counting
  m_buffer.Set(buffer);
  return true;
}
}  // namespace karate
}  // namespace plugin
}  // namespace ola
