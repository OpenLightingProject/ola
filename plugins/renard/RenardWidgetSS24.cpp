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
 * RenardWidgetSS24.cpp
 * The Renard SS24 Widget.
 * Copyright (C) 2013 Hakan Lindestaf
 */

#include <algorithm>

#include "ola/Logging.h"
#include "plugins/renard/RenardWidgetSS24.h"

namespace ola {
namespace plugin {
namespace renard {

/*
 * Connect to the widget
 */
bool RenardWidgetSS24::Connect() {
  OLA_DEBUG << "Connecting to " << m_path;

  int fd = ConnectToWidget(m_path);

  if (fd < 0)
    return false;

  m_socket = new ola::io::DeviceDescriptor(fd);

  OLA_DEBUG << "Connected to " << m_path;
  return true;
}


/*
 * Check if this is actually a Renard device
 * @return true if this is a renard,  false otherwise
 */
bool RenardWidgetSS24::DetectDevice() {
  // This device doesn't do two way comms, so just return true
  return true;
}


/*
 * Send a DMX msg.
  */
bool RenardWidgetSS24::SendDmx(const DmxBuffer &buffer) const {
  // TODO(Peter): Probably add offset in here to send higher channels shifted
  // down
  int bytes_sent = Send112(buffer);
  OLA_DEBUG << "Sending DMX, sent " << bytes_sent << " bytes";
  // Should this confirm we've sent more than 0 bytes and return false if not?
  return true;
}


//-----------------------------------------------------------------------------
// Private methods used for communicating with the widget
/*
 * Set a single channel
 */
int RenardWidgetSS24::SetChannel(unsigned int chan, uint8_t val) const {
  uint8_t msg[2];

  msg[0] = chan;
  msg[1] = val;
  OLA_DEBUG << "Setting " << chan << " to " << static_cast<int>(val);
  return m_socket->Send(msg, sizeof(msg));
}


/*
 * Send 112 channels worth of data
 * @param buf a DmxBuffer with the data
 */
int RenardWidgetSS24::Send112(const DmxBuffer &buffer) const {
  unsigned int channels = std::min((unsigned int) DMX_MAX_TRANSMIT_CHANNELS,
                                   buffer.Size());
  uint8_t msg[channels * 2];

  for (unsigned int i = 0; i <= channels; i++) {
    msg[i * 2] = i + 1;
    msg[(i * 2) + 1] = buffer.Get(i);
    OLA_DEBUG << "Setting " << (i + 1) << " to " <<
        static_cast<int>(buffer.Get(i));
  }
  return m_socket->Send(msg, (channels * 2));
}
}  // namespace renard
}  // namespace plugin
}  // namespace ola
