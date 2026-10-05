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
 * BaseUsbProWidget.cpp
 * Read and Write to a USB Widget.
 * Copyright (C) 2010 Simon Newton
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <string>
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/io/IOUtils.h"
#include "ola/io/Serial.h"
#include "ola/base/Macro.h"
#include "plugins/usbpro/BaseUsbProWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::string;


const unsigned int BaseUsbProWidget::HEADER_SIZE =
  sizeof(BaseUsbProWidget::message_header);


BaseUsbProWidget::BaseUsbProWidget(
    ola::io::ConnectedDescriptor *descriptor)
    : m_descriptor(descriptor),
      m_state(PRE_SOM),
      m_bytes_received(0) {
  memset(&m_header, 0, sizeof(m_header));
  m_descriptor->SetOnData(
      NewCallback(this, &BaseUsbProWidget::DescriptorReady));
}


BaseUsbProWidget::~BaseUsbProWidget() {
  m_descriptor->SetOnData(NULL);
}


/*
 * Read data from the widget
 */
void BaseUsbProWidget::DescriptorReady() {
  while (m_descriptor->DataRemaining() > 0) {
    ReceiveMessage();
  }
}


/*
 * Send a DMX frame.
 * @returns true if we sent ok, false otherwise
 */
bool BaseUsbProWidget::SendDMX(const DmxBuffer &buffer) {
  struct {
    uint8_t start_code;
    uint8_t dmx[DMX_UNIVERSE_SIZE];
  } widget_dmx;

  widget_dmx.start_code = DMX512_START_CODE;
  unsigned int length = DMX_UNIVERSE_SIZE;
  buffer.Get(widget_dmx.dmx, &length);
  return SendMessage(DMX_LABEL,
                     reinterpret_cast<uint8_t*>(&widget_dmx),
                     length + 1);
}


/*
 * Send the msg
 * @return true if successful, false otherwise
 */
bool BaseUsbProWidget::SendMessage(uint8_t label,
                                   const uint8_t *data,
                                   unsigned int length) const {
  if (length && !data)
    return false;

  ssize_t frame_size = HEADER_SIZE + length + 1;
  uint8_t frame[frame_size];
  message_header *header = reinterpret_cast<message_header*>(frame);
  header->som = SOM;
  header->label = label;
  header->len = length & 0xFF;
  header->len_hi = (length & 0xFF00) >> 8;

  memcpy(frame + sizeof(message_header), data, length);
  frame[frame_size - 1] = EOM;

  ssize_t bytes_sent = m_descriptor->Send(frame, frame_size);
  if (bytes_sent != frame_size)
    // we've probably screwed framing at this point
    return false;

  return true;
}


/**
 * Open a path and apply the settings required for talking to widgets.
 */
ola::io::ConnectedDescriptor *BaseUsbProWidget::OpenDevice(
    const string &path) {
  struct termios newtio;
  int fd;
  if (!ola::io::AcquireLockAndOpenSerialPort(path,
                                             O_RDWR | O_NONBLOCK | O_NOCTTY,
                                             &fd)) {
    return NULL;
  }

  bzero(&newtio, sizeof(newtio));  // clear struct for new port settings
  newtio.c_cflag |= CREAD;
  newtio.c_cflag |= CS8;
  cfsetispeed(&newtio, B115200);
  cfsetospeed(&newtio, B115200);
  tcsetattr(fd, TCSANOW, &newtio);

  return new ola::io::DeviceDescriptor(fd);
}

/*
 * Read the data and handle the messages.
 */
void BaseUsbProWidget::ReceiveMessage() {
  unsigned int count, packet_length;

  switch (m_state) {
    case PRE_SOM:
      do {
        m_descriptor->Receive(&m_header.som, 1, count);
        if (count != 1)
          return;
      } while (m_header.som != SOM);
      m_state = RECV_LABEL;
      // fall through
      OLA_FALLTHROUGH
    case RECV_LABEL:
      m_descriptor->Receive(&m_header.label, 1, count);
      if (count != 1)
        return;
      m_state = RECV_SIZE_LO;
      // fall through
      OLA_FALLTHROUGH
    case RECV_SIZE_LO:
      m_descriptor->Receive(&m_header.len, 1, count);
      if (count != 1)
        return;
      m_state = RECV_SIZE_HI;
      // fall through
      OLA_FALLTHROUGH
    case RECV_SIZE_HI:
      m_descriptor->Receive(&m_header.len_hi, 1, count);
      if (count != 1)
        return;

      packet_length = (m_header.len_hi << 8) + m_header.len;
      if (packet_length == 0) {
        m_state = RECV_EOM;
        return;
      } else if (packet_length > MAX_DATA_SIZE) {
        m_state = PRE_SOM;
        return;
      }

      m_bytes_received = 0;
      m_state = RECV_BODY;
      // fall through
      OLA_FALLTHROUGH
    case RECV_BODY:
      packet_length = (m_header.len_hi << 8) + m_header.len;
      m_descriptor->Receive(
          reinterpret_cast<uint8_t*>(&m_recv_buffer) + m_bytes_received,
          packet_length - m_bytes_received,
          count);

      if (!count)
        return;

      m_bytes_received += count;
      if (m_bytes_received != packet_length)
        return;

      m_state = RECV_EOM;
      // fall through
      OLA_FALLTHROUGH
    case RECV_EOM:
      // check this is a valid frame with an end byte
      uint8_t eom;
      m_descriptor->Receive(&eom, 1, count);
      if (count != 1)
        return;

      packet_length = (m_header.len_hi << 8) + m_header.len;

      if (eom == EOM)
        HandleMessage(m_header.label,
                      packet_length ? m_recv_buffer : NULL,
                      packet_length);
      m_state = PRE_SOM;
  }
  return;
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
