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
 * RenardWidgetSS8.cpp
 * The Renard SS8 Widget.
 * Copyright (C) 2013 Hakan Lindestaf
 */

#include <algorithm>

#include "ola/Logging.h"
#include "ola/io/IOUtils.h"
#include "plugins/renard/RenardWidgetSS8.h"

namespace ola {
namespace plugin {
namespace renard {

// Based on standard Renard firmware
const uint8_t RenardWidgetSS8::RENARD_START_ADDRESS = 0x80;
const uint8_t RenardWidgetSS8::RENARD_COMMAND_PAD = 0x7D;
const uint8_t RenardWidgetSS8::RENARD_COMMAND_START_PACKET = 0x7E;
const uint8_t RenardWidgetSS8::RENARD_COMMAND_ESCAPE = 0x7F;
const uint8_t RenardWidgetSS8::RENARD_CHANNELS_IN_BANK = 8;

/*
 * Connect to the widget
 */
bool RenardWidgetSS8::Connect() {
  OLA_DEBUG << "Connecting to " << m_path;
  OLA_DEBUG << "Baudrate set to " << static_cast<int>(m_baudrate);

  speed_t baudrate;
  if (!ola::io::IntegerToSpeedT(m_baudrate, &baudrate)) {
    OLA_DEBUG << "Failed to convert baudrate, i.e. not supported baud rate";
    return false;
  }

  int fd = ConnectToWidget(m_path, baudrate);

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
bool RenardWidgetSS8::DetectDevice() {
  // This device doesn't do two way comms, so just return true
  return true;
}


/*
 * Send a DMX msg.
  */
bool RenardWidgetSS8::SendDmx(const DmxBuffer &buffer) {
  unsigned int channels = std::min((unsigned int) m_channels + m_dmxOffset,
                                   buffer.Size()) - m_dmxOffset;

  // Max buffer size for worst case scenario
  unsigned int bufferSize = channels * 2 + 10;
  uint8_t msg[bufferSize];

  int dataToSend = 0;

  for (unsigned int i = 0; i < channels; i++) {
    if ((i % RENARD_CHANNELS_IN_BANK) == 0) {
      if (byteCounter >= 100) {
        // Send PAD
        msg[dataToSend++] = RENARD_COMMAND_PAD;
        byteCounter = 0;
      }

      // Send address
      msg[dataToSend++] = RENARD_COMMAND_START_PACKET;
      msg[dataToSend++] = m_startAddress + (i / 8);
      byteCounter += 2;
    }

    uint8_t b = buffer.Get(m_dmxOffset + i);

    // Escaping magic bytes
    switch (b) {
      case RENARD_COMMAND_PAD:
        msg[dataToSend++] = RENARD_COMMAND_ESCAPE;
        msg[dataToSend++] = 0x2F;
        byteCounter += 2;
        break;

      case RENARD_COMMAND_START_PACKET:
        msg[dataToSend++] = RENARD_COMMAND_ESCAPE;
        msg[dataToSend++] = 0x30;
        byteCounter += 2;
        break;

      case RENARD_COMMAND_ESCAPE:
        msg[dataToSend++] = RENARD_COMMAND_ESCAPE;
        msg[dataToSend++] = 0x31;
        byteCounter += 2;
        break;

      default:
        msg[dataToSend++] = b;
        byteCounter++;
        break;
    }

    OLA_DEBUG << "Setting " << (i + 1) << " to " <<
        static_cast<int>(b);
  }

  int bytes_sent = m_socket->Send(msg, dataToSend);

  OLA_DEBUG << "Sending DMX, sent " << bytes_sent << " bytes";

  return true;
}
}  // namespace renard
}  // namespace plugin
}  // namespace ola
