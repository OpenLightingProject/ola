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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * MilInstWidget1463.cpp
 * The MilInst 1-463 Widget.
 * Copyright (C) 2013 Peter Newman
 */

#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <algorithm>
#include <string>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/io/Descriptor.h"
#include "plugins/milinst/MilInstWidget1463.h"

namespace ola {
namespace plugin {
namespace milinst {

/*
 * Connect to the widget
 */
bool MilInstWidget1463::Connect(const std::string &path) {
  struct termios newtio;

  OLA_DEBUG << "Connecting to " << path;

  int fd = open(path.data(), O_RDWR | O_NONBLOCK | O_NOCTTY);

  if (fd == -1)
    return false;

  memset(&newtio, 0, sizeof(newtio));  // clear struct for new port settings
  tcgetattr(fd, &newtio);
  newtio.c_cflag |= (CLOCAL | CREAD);  // Enable read
  newtio.c_cflag |= CS8;  // 8n1
  newtio.c_cflag &= ~CRTSCTS;  // No flow control
  cfsetospeed(&newtio, B9600);
  tcsetattr(fd, TCSANOW, &newtio);
  m_socket = new ola::io::DeviceDescriptor(fd);

  OLA_DEBUG << "Connected to " << path;
  m_path = path;
  return true;
}


/*
 * Check if this is actually a MilInst device
 * @return true if this is a milinst,  false otherwise
 */
bool MilInstWidget1463::DetectDevice() {
  // This device doesn't do two way comms, so just return true
  return true;
}


/*
 * Send a dmx msg.
  */
bool MilInstWidget1463::SendDmx(const DmxBuffer &buffer) const {
  // TODO(Peter): Make this use Send112 instead
  OLA_DEBUG << "Sending DMX";
  for (int n = 1; n <= DMX_MAX_TRANSMIT_CHANNELS; n++) {
    OLA_DEBUG << "Setting " << n << " to " <<
    static_cast<int>(buffer.Get(n - 1)) << ", sent " <<
    SetChannel(n, buffer.Get(n - 1)) << " bytes";
  }
  // unsigned int index = 0;
  // while (index < buffer.Size()) {
  //   unsigned int size = std::min((unsigned int) DMX_MAX_TRANSMIT,
  //                                buffer.Size() - index);
  //   Send112(index, buffer.GetRaw() + index, size);
  //   index += size;
  // }
  return true;
}


//-----------------------------------------------------------------------------
// Private methods used for communicating with the widget
/*
 * Set a single channel
 */
int MilInstWidget1463::SetChannel(unsigned int chan, uint8_t val) const {
  uint8_t msg[2];

  msg[0] = chan;
  msg[1] = val;
  OLA_DEBUG << "Setting " << chan << " to " << static_cast<int>(val);
  return m_socket->Send(msg, sizeof(msg));
}


/*
 * Send 112 channels worth of data
 * @param start the start channel for the data
 * @param buf a pointer to the data
 * @param len the length of the data
 */
int MilInstWidget1463::Send112(unsigned int start, const uint8_t *buf,
                              unsigned int length) const {
  // TODO(Peter): Make this work!
  unsigned int len = std::min((unsigned int) DMX_MAX_TRANSMIT_CHANNELS, length);
  uint8_t msg[DMX_MAX_TRANSMIT_CHANNELS * 2];

  msg[0] = start + len;
  memcpy(msg + DMX_MAX_TRANSMIT_CHANNELS, buf, len);
  return m_socket->Send(msg, len + DMX_MAX_TRANSMIT_CHANNELS);
}
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
