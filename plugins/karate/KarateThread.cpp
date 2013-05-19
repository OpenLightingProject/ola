/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * KarateThread.h
 * Thread for the karate device
 * Copyright (C) 2005-2007  Simon Newton
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

#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "plugins/karate/KarateThread.h"
#include "plugins/karate/KarateLight.h"

namespace ola {
namespace plugin {
namespace karate {

using std::string;
using ola::thread::Mutex;
using ola::thread::MutexLocker;

/*
 * Create a new KarateThread object
 */
KarateThread::KarateThread(const string &path)
    : ola::thread::Thread(),
    m_fd(INVALID_FD),
    m_path(path),
    m_term(false) {
}


/*
 * Run this thread
 */
void *KarateThread::Run() {
  unsigned int dmx_offset = 0;
  unsigned int length = DMX_UNIVERSE_SIZE;

  struct timeval tv;
  struct timespec ts;

  // should close other fd here
  KarateLight k(m_path);
  k.Init();
  dmx_offset = k.GetDMXOffset();
  OLA_INFO << "DMX_OFFSET " << m_path << ": " << dmx_offset;

  while (true) {
    {
      MutexLocker lock(&m_term_mutex);
      if (m_term)
        break;
    }

    if (!k.IsActive()) {
      // try to reopen the device...
      if (gettimeofday(&tv, NULL) < 0) {
        OLA_WARN << "gettimeofday error";
        break;
      }
      ts.tv_sec = tv.tv_sec + 1;
      ts.tv_nsec = tv.tv_usec * 1000;

      // wait for either a signal that we should terminate, or ts seconds
      m_term_mutex.Lock();
      if (m_term)
        break;
      m_term_cond.TimedWait(&m_term_mutex, &ts);
      m_term_mutex.Unlock();

      OLA_WARN << "Re-Initialising device " << m_path;
      k.Init();
      dmx_offset = k.GetDMXOffset();

    } else {
      length = DMX_UNIVERSE_SIZE;
      {
        MutexLocker locker(&m_mutex);
        k.SetColors(m_buffer);
      }
      if (k.UpdateColors() != KarateLight::KL_OK) {
         OLA_WARN << "Failed to write color data";
      }  else {
          usleep(20000);  // 50Hz
      }
    }  // port is okay
  }
  return NULL;
}


/*
 * Stop the thread
 */
bool KarateThread::Stop() {
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
bool KarateThread::WriteDmx(const DmxBuffer &buffer) {
  MutexLocker locker(&m_mutex);
  // avoid the reference counting
  m_buffer.Set(buffer);
  return true;
}
}  // namespace karate
}  // namespace plugin
}  // namespace ola
