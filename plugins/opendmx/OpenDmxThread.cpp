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
 * OpenDmxThread.cpp
 * Thread for the Open DMX device
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
#include <time.h>
#include <unistd.h>
#include <string>

#include "ola/Clock.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/io/IOUtils.h"
#include "plugins/opendmx/OpenDmxThread.h"

namespace ola {
namespace plugin {
namespace opendmx {

using std::string;
using ola::thread::Mutex;
using ola::thread::MutexLocker;

/*
 * Create a new OpenDmxThread object
 */
OpenDmxThread::OpenDmxThread(const string &path)
    : ola::thread::Thread(),
    m_fd(INVALID_FD),
    m_path(path),
    m_term(false) {
}


/*
 * Run this thread
 */
void *OpenDmxThread::Run() {
  uint8_t buffer[DMX_UNIVERSE_SIZE+1];
  unsigned int length = DMX_UNIVERSE_SIZE;
  Clock clock;

  // should close other fd here

  // start code
  buffer[0] = 0x00;
  ola::io::Open(m_path, O_WRONLY, &m_fd);

  while (true) {
    {
      MutexLocker lock(&m_term_mutex);
      if (m_term)
        break;
    }

    if (m_fd == INVALID_FD) {
      TimeStamp wake_up;
      // Use real time here because wake_up is passed to pthread_cond_timedwait
      clock.CurrentRealTime(&wake_up);
      wake_up += TimeInterval(1, 0);

      // wait for either a signal that we should terminate, or ts seconds
      m_term_mutex.Lock();
      if (m_term)
        break;
      m_term_cond.TimedWait(&m_term_mutex, wake_up);
      m_term_mutex.Unlock();

      ola::io::Open(m_path, O_WRONLY, &m_fd);

    } else {
      length = DMX_UNIVERSE_SIZE;
      {
        MutexLocker locker(&m_mutex);
        m_buffer.Get(buffer + 1, &length);
      }

      if (write(m_fd, buffer, length + 1) < 0) {
        // if you unplug the dongle
        OLA_WARN << "Error writing to device: " << strerror(errno);

        if (close(m_fd) < 0)
          OLA_WARN << "Close failed " << strerror(errno);
        m_fd = INVALID_FD;
      }
    }
  }
  return NULL;
}


/*
 * Stop the thread
 */
bool OpenDmxThread::Stop() {
  {
    MutexLocker locker(&m_mutex);
    m_term = true;
  }
  m_term_cond.Signal();
  return Join();
}


/*
 * Store the data in the shared buffer
 *
 */
bool OpenDmxThread::WriteDmx(const DmxBuffer &buffer) {
  MutexLocker locker(&m_mutex);
  // avoid the reference counting
  m_buffer.Set(buffer);
  return true;
}
}  // namespace opendmx
}  // namespace plugin
}  // namespace ola
