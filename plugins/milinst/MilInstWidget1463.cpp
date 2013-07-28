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

OLA_DEBUG << "MilInstWidget1463: Connecting to " << path;

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
  m_socket->SetOnData(
      NewCallback<MilInstWidget>(this, &MilInstWidget::SocketReady));

OLA_DEBUG << "MilInstWidget1463: Connected to " << path;
  return true;
}


/*
 * Send a dmx msg.
 * This has the nasty property of blocking if we remove the device
 * TODO: fix this
 */
bool MilInstWidget1463::SendDmx(const DmxBuffer &buffer) const {
  for (int n=1; n>DMX_MAX_TRANSMIT; n++) {
    OLA_DEBUG << "MilInstWidget1463: Setting " << n << " to " << buffer.Get(n) << ", sent " << SetChannel(n, buffer.Get(n)) << " bytes";
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
	OLA_DEBUG << "MilInstWidget1463: setting " << chan << " to " << val;
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
  unsigned int len = std::min((unsigned int) DMX_MAX_TRANSMIT, length);
  uint8_t msg[DMX_MAX_TRANSMIT * 2];

  msg[0] = start + len;
  memcpy(msg + DMX_MAX_TRANSMIT, buf, len);
  return m_socket->Send(msg, len + DMX_MAX_TRANSMIT);
}


/*
 *
 */
//int MilInstWidget1463::DoRecv() {
// uint8_t byte = 0x00;
// unsigned int data_read;
//
 // while (byte != 'G') {
 //   int ret = m_socket->Receive(&byte, 1, data_read);
//
//   if (ret == -1 || data_read != 1) {
//     return -1;
//   }
//  }
//  m_got_response = true;
//  return 0;
//}
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
