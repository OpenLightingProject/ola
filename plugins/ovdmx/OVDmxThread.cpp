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
 * OVDmxThread.cpp
 * Thread for the OVDMX device
 * Copyright (C) 2005 Simon Newton, 2017 Jan Ove Saltvedt
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
#include <termios.h>

#include "ola/Clock.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/io/IOUtils.h"
#include "plugins/ovdmx/OVDmxThread.h"

namespace ola {
namespace plugin {
namespace ovdmx {

using std::string;
using ola::thread::Mutex;
using ola::thread::MutexLocker;

/*
 * Create a new OVDmxThread object
 */
OVDmxThread::OVDmxThread(const string &path)
    : ola::thread::Thread(),
    m_fd(INVALID_FD),
    m_path(path),
    m_term(false) {
}


/*
 * Strips away tty rewriting of \n to \r\n (bad for binary transfers)
 */

void OVDmxThread::MakeRaw(int fd) {
    struct termios ios;
    //Not a TTY: nothing to do
    if (!isatty(fd))
        return;

    tcgetattr(fd, &ios);
    cfmakeraw(&ios);
    tcsetattr(fd, TCSANOW, &ios);

    return;
}

/*
 * Run this thread
 */
void *OVDmxThread::Run() {
  struct __attribute__((__packed__))  {
    uint8_t magic[2];
    uint8_t type;
    union {
      uint16_t data_length;
      uint8_t data_length_parts[2];
    };
    uint8_t data[DMX_UNIVERSE_SIZE];
    union {
      uint16_t crc;
      uint8_t crc_parts[2];
    };
  } dmx_packet;

  dmx_packet.magic[0] = 'O';
  dmx_packet.magic[1] = 'V';
  dmx_packet.type  = 'D';
  dmx_packet.data_length_parts[0] = ((((uint16_t)DMX_UNIVERSE_SIZE)>>8) & 0xff);
  dmx_packet.data_length_parts[1] = ((((uint16_t)DMX_UNIVERSE_SIZE)) & 0xff);
  dmx_packet.crc_parts[0] = 0;
  dmx_packet.crc_parts[1] = 0;

  Clock clock;

  // should close other fd here

  ola::io::Open(m_path, O_WRONLY, &m_fd);
  MakeRaw(m_fd);
  while (true) {
    {
      MutexLocker lock(&m_term_mutex);
      if (m_term)
        break;
    }

    if (m_fd == INVALID_FD) {
      TimeStamp wake_up;
      clock.CurrentTime(&wake_up);
      wake_up += TimeInterval(1, 0);

      // wait for either a signal that we should terminate, or ts seconds
      m_term_mutex.Lock();
      if (m_term)
        break;
      m_term_cond.TimedWait(&m_term_mutex, wake_up);
      m_term_mutex.Unlock();

      ola::io::Open(m_path, O_WRONLY, &m_fd);
      MakeRaw(m_fd);

    } else {
      unsigned int length = DMX_UNIVERSE_SIZE;
      {
        MutexLocker locker(&m_mutex);
        m_buffer.Get(dmx_packet.data , &length);
      }

      if (write(m_fd, &dmx_packet, sizeof(dmx_packet)) < 0) {
        // if you unplug the dongle
        OLA_WARN << "Error writing to device: " << strerror(errno);

        if (close(m_fd) < 0)
          OLA_WARN << "Close failed " << strerror(errno);
        m_fd = INVALID_FD;
      }
      else {
          fsync(m_fd);
      }
    }
  }
  return NULL;
}


/*
 * Stop the thread
 */
bool OVDmxThread::Stop() {
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
bool OVDmxThread::WriteDmx(const DmxBuffer &buffer) {
  MutexLocker locker(&m_mutex);
  // avoid the reference counting
  m_buffer.Set(buffer);
  return true;
}
}  // namespace ovdmx
}  // namespace plugin
}  // namespace ola
