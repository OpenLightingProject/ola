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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>

#include <llad/logger.h>
#include "OpenDmxThread.h"

namespace lla {
namespace plugin {

using std::string;

typedef struct {
  OpenDmxThread *th;
  string path;
} t_args;


void *thread_run(void *d) {
  t_args *args = (t_args*) d;

  args->th->Run(args->path);
  free(args);
}

/*
 * Create a new OpenDmxThread object
 *
 */
OpenDmxThread::OpenDmxThread() {
  m_fd = -1;
  memset(m_dmx, 0x00, DMX_UNIVERSE_SIZE);
  pthread_mutex_init(&m_mutex, NULL);
  m_term = false;
  pthread_mutex_init(&m_term_mutex, NULL);
  pthread_cond_init(&m_term_cond, NULL);

  m_tid = 0;
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
 * run this thread
 *
 */
void *OpenDmxThread::Run(const string &path) {
  uint8_t buf[DMX_UNIVERSE_SIZE+1];
  struct timeval tv;
  struct timespec ts;

  // should close other fd here

  // start code
  buf[0] = 0x00;
  m_fd = open(path.c_str(), O_WRONLY);

  while (1) {
    pthread_mutex_lock(&m_term_mutex);
    if (m_term) {
      pthread_mutex_unlock(&m_term_mutex);
      break;
    }
    pthread_mutex_unlock(&m_term_mutex);

    if (m_fd == -1) {
      if (gettimeofday(&tv, NULL) < 0) {
        printf("gettimeofday error\n");
        break;
      }
      ts.tv_sec = tv.tv_sec + 1;
      ts.tv_nsec = tv.tv_usec * 1000;

      pthread_cond_timedwait(&m_term_cond, &m_term_mutex, &ts);
      pthread_mutex_unlock(&m_term_mutex);

      m_fd = open(path.c_str(), O_WRONLY);

      if (m_fd == -1)
        printf("open %d: %s\n", m_fd, strerror(errno));

    } else {
      pthread_mutex_lock(&m_mutex);
      memcpy(&buf[1], m_dmx, DMX_UNIVERSE_SIZE);
      pthread_mutex_unlock(&m_mutex);

      do_write(buf, DMX_UNIVERSE_SIZE + 1);
    }
  }
  return NULL;
}


/*
 * Start this thread
 *
 */
int OpenDmxThread::Start(const string &path) {
  // this is passed to the thread and free'ed there
  t_args *args = (t_args*) malloc(sizeof(t_args));

  args->th = this;
  args->path = path;

  if (pthread_create(&m_tid, NULL, lla::plugin::thread_run, (void*) args)) {
    Logger::instance()->log(Logger::WARN, "pthread create failed");
    return -1;
  }
  return 0;
}


/*
 * Stop the thread
 *
 */
int OpenDmxThread::Stop() {
  pthread_mutex_lock(&m_term_mutex);
  m_term = true;
  pthread_mutex_unlock(&m_term_mutex);

  pthread_cond_signal(&m_term_cond);
  pthread_join(m_tid, NULL);
}



/*
 * Store the data in the shared buffer
 *
 */
int OpenDmxThread::WriteDmx(uint8_t *data, int length) {
  pthread_mutex_lock(&m_mutex);
  memcpy(m_dmx, data, length);
  pthread_mutex_unlock(&m_mutex);
}



int OpenDmxThread::do_write(uint8_t *buf, int length) {
  int res = write(m_fd, buf, length);

  if (res < 0) {
    // if you unplug devices from the dongle
    perror("Error writing to device");

    res = close(m_fd);
    if (res < 0)
      perror("close");
    else
      m_fd = -1;

    return -1;
  }
  return 0;
}

} // plugin
} // lla
