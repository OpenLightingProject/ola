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
 * OpenDmxThread.h
 * Thread for the open dmx device
 * Copyright (C) 2005-2007  Simon Newton
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <string>

#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "plugins/opendmx/OpenDmxThread.h"

namespace ola {
namespace plugin {
namespace opendmx {

using std::string;

/*
 * Create a new OpenDmxThread object
 */
OpenDmxThread::OpenDmxThread(const string &path)
    : OlaThread(),
    m_fd(-1),
    m_path(path),
    m_term(false) {
  pthread_mutex_init(&m_mutex, NULL);
  pthread_mutex_init(&m_term_mutex, NULL);
  pthread_cond_init(&m_term_cond, NULL);
}


/*
 *
 */
OpenDmxThread::~OpenDmxThread() {
  pthread_cond_destroy(&m_term_cond);
  pthread_mutex_destroy(&m_term_mutex);
  pthread_mutex_destroy(&m_mutex);
}


/*
 * Run this thread
 */
void *OpenDmxThread::Run() {
  uint8_t buffer[DMX_UNIVERSE_SIZE+1];
  unsigned int length = DMX_UNIVERSE_SIZE;
  struct timeval tv;
  struct timespec ts;

  // should close other fd here

  // start code
  buffer[0] = 0x00;
  m_fd = open(m_path.c_str(), O_WRONLY);

  while (true) {
    pthread_mutex_lock(&m_term_mutex);
    if (m_term) {
      pthread_mutex_unlock(&m_term_mutex);
      break;
    }
    pthread_mutex_unlock(&m_term_mutex);

    if (m_fd == -1) {
      if (gettimeofday(&tv, NULL) < 0) {
        OLA_WARN << "gettimeofday error";
        break;
      }
      ts.tv_sec = tv.tv_sec + 1;
      ts.tv_nsec = tv.tv_usec * 1000;

      pthread_cond_timedwait(&m_term_cond, &m_term_mutex, &ts);
      pthread_mutex_unlock(&m_term_mutex);

      m_fd = open(m_path.c_str(), O_WRONLY);

      if (m_fd == -1)
        OLA_WARN << "Open " << m_fd << ": " << strerror(errno);

    } else {
      length = DMX_UNIVERSE_SIZE;
      pthread_mutex_lock(&m_mutex);
      m_buffer.Get(buffer + 1, &length);
      pthread_mutex_unlock(&m_mutex);

      if (write(m_fd, buffer, length + 1) < 0) {
        // if you unplug the dongle
        OLA_WARN << "Error writing to device: " << strerror(errno);

        if (close(m_fd) < 0)
          OLA_WARN << "Close failed " << strerror(errno);
        m_fd = -1;
      }
    }
  }
  return NULL;
}


/*
 * Stop the thread
 */
bool OpenDmxThread::Stop() {
  pthread_mutex_lock(&m_term_mutex);
  m_term = true;
  pthread_mutex_unlock(&m_term_mutex);

  pthread_cond_signal(&m_term_cond);
  return Join();
}


/*
 * Store the data in the shared buffer
 *
 */
bool OpenDmxThread::WriteDmx(const DmxBuffer &buffer) {
  pthread_mutex_lock(&m_mutex);
  m_buffer = buffer;
  pthread_mutex_unlock(&m_mutex);
  return true;
}
}  // opendmx
}  // plugin
}  // ola
