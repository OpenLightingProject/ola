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
 * RenardWidget.cpp
 * This is the Renard widget class
 * Copyright (C) 2013 Hakan Lindestaf
 */

#include <algorithm>
#include <string>

#include "ola/Logging.h"
#include "ola/io/IOUtils.h"
#include "plugins/renard/RenardWidget.h"

namespace ola {
namespace plugin {
namespace renard {

// Based on standard Renard firmware
const uint8_t RenardWidget::RENARD_COMMAND_PAD = 0x7D;
const uint8_t RenardWidget::RENARD_COMMAND_START_PACKET = 0x7E;
const uint8_t RenardWidget::RENARD_COMMAND_ESCAPE = 0x7F;
const uint8_t RenardWidget::RENARD_ESCAPE_PAD = 0x2F;
const uint8_t RenardWidget::RENARD_ESCAPE_START_PACKET = 0x30;
const uint8_t RenardWidget::RENARD_ESCAPE_ESCAPE = 0x31;
// The Renard protocol is built around 8 channels per packet
const uint8_t RenardWidget::RENARD_CHANNELS_IN_BANK = 8;
// Discussions on the Renard firmware recommended a padding each 100 bytes or so
const uint32_t RenardWidget::RENARD_BYTES_BETWEEN_PADDING = 100;

/*
 * New widget
 */
RenardWidget::~RenardWidget() {
  if (m_socket) {
    m_socket->Close();
    delete m_socket;
  }
}

/*
 * Connect to the widget
 */
bool RenardWidget::Connect() {
  OLA_DEBUG << "Connecting to " << m_path;
  OLA_DEBUG << "Baudrate set to " << static_cast<int>(m_baudrate);

  speed_t baudrate;
  if (!ola::io::UIntToSpeedT(m_baudrate, &baudrate)) {
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
 * Connect to the widget
 */
int RenardWidget::ConnectToWidget(const std::string &path, speed_t speed) {
  struct termios newtio;

  if (path.empty()) {
    OLA_DEBUG << "No path configured for device, please set one in "
        "ola-renard.conf";
    return -1;
  }

  int fd = open(path.data(), O_RDWR | O_NONBLOCK | O_NOCTTY);

  if (fd == -1)
    return -1;

  memset(&newtio, 0, sizeof(newtio));  // Clear struct for new port settings
  tcgetattr(fd, &newtio);
  newtio.c_cflag |= (CLOCAL | CREAD);  // Enable read
  newtio.c_cflag |= CS8;  // 8n1
  newtio.c_cflag &= ~CRTSCTS;  // No flow control
  cfsetispeed(&newtio, speed);
  cfsetospeed(&newtio, speed);
  tcsetattr(fd, TCSANOW, &newtio);

  return fd;
}


/*
 * Disconnect from the widget
 */
int RenardWidget::Disconnect() {
  m_socket->Close();
  return 0;
}


/*
 * Check if this is actually a Renard device
 * @return true if this is a renard,  false otherwise
 */
bool RenardWidget::DetectDevice() {
  // This device doesn't do two way comms, so just return true
  return true;
}


/*
 * Send a DMX msg.
 */
bool RenardWidget::SendDmx(const DmxBuffer &buffer) {
  unsigned int channels = std::max((unsigned int)0,
                                   std::min((unsigned int) m_channels +
                                            m_dmxOffset, buffer.Size()) -
                                   m_dmxOffset);

  OLA_DEBUG << "Sending " << static_cast<int>(channels) << " channels";

  // Max buffer size for worst case scenario (escaping + padding)
  unsigned int bufferSize = channels * 2 + 10;
  uint8_t msg[bufferSize];

  int dataToSend = 0;

  for (unsigned int i = 0; i < channels; i++) {
    if ((i % RENARD_CHANNELS_IN_BANK) == 0) {
      if (m_byteCounter >= RENARD_BYTES_BETWEEN_PADDING) {
        // Send PAD every 100 (or so) bytes. Note that the counter is per
        // device, so the counter should span multiple calls to SendDMX.
        msg[dataToSend++] = RENARD_COMMAND_PAD;
        m_byteCounter = 0;
      }

      // Send address
      msg[dataToSend++] = RENARD_COMMAND_START_PACKET;
      msg[dataToSend++] = m_startAddress + (i / RENARD_CHANNELS_IN_BANK);
      m_byteCounter += 2;
    }

    uint8_t b = buffer.Get(m_dmxOffset + i);

    // Escaping magic bytes
    switch (b) {
      case RENARD_COMMAND_PAD:
        msg[dataToSend++] = RENARD_COMMAND_ESCAPE;
        msg[dataToSend++] = RENARD_ESCAPE_PAD;
        m_byteCounter += 2;
        break;

      case RENARD_COMMAND_START_PACKET:
        msg[dataToSend++] = RENARD_COMMAND_ESCAPE;
        msg[dataToSend++] = RENARD_ESCAPE_START_PACKET;
        m_byteCounter += 2;
        break;

      case RENARD_COMMAND_ESCAPE:
        msg[dataToSend++] = RENARD_COMMAND_ESCAPE;
        msg[dataToSend++] = RENARD_ESCAPE_ESCAPE;
        m_byteCounter += 2;
        break;

      default:
        msg[dataToSend++] = b;
        m_byteCounter++;
        break;
    }

    OLA_DEBUG << "Setting Renard " << m_startAddress +
      (i / RENARD_CHANNELS_IN_BANK) << "/" <<
      ((i % RENARD_CHANNELS_IN_BANK) + 1) << " to " <<
      static_cast<int>(b);
  }

  int bytes_sent = m_socket->Send(msg, dataToSend);

  OLA_DEBUG << "Sending DMX, sent " << bytes_sent << " bytes";

  return true;
}
}  // namespace renard
}  // namespace plugin
}  // namespace ola
