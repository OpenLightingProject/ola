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
 * RobeUsbWidget.cpp
 * Read and Write to a Robe USB Widget.
 * Copyright (C) 2011 Simon Newton
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
#include "ola/Logging.h"
#include "plugins/usbpro/RobeUsbWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {


RobeUsbWidget::RobeUsbWidget(ola::network::ConnectedDescriptor *descriptor)
    : m_callback(NULL),
      m_descriptor(descriptor),
      m_state(PRE_SOM),
      m_bytes_received(0) {
  m_descriptor->SetOnData(NewCallback(this, &RobeUsbWidget::DescriptorReady));
}


RobeUsbWidget::~RobeUsbWidget() {
  if (m_callback)
    delete m_callback;
  // don't delete because ownership is transferred to the ss so that device
  // removal works correctly. If you delete the descriptor the OnClose closure
  // will be deleted, which breaks if it's already running.
  m_descriptor->SetOnClose(NULL);
  m_descriptor->Close();
  m_descriptor = NULL;
}


/**
 * Set the closure to be called when we receive a message from the widget
 * @param callback the closure to run, ownership is transferred.
 */
void RobeUsbWidget::SetMessageHandler(
    ola::Callback3<void, uint8_t, const uint8_t*, unsigned int> *callback) {
  if (m_callback)
    delete m_callback;
  m_callback = callback;
}


/*
 * Set the onRemove handler
 */
void RobeUsbWidget::SetOnRemove(ola::SingleUseCallback0<void> *on_close) {
  m_descriptor->SetOnClose(on_close);
}


/*
 * Read data from the widget
 */
void RobeUsbWidget::DescriptorReady() {
  while (m_descriptor->DataRemaining() > 0) {
    ReceiveMessage();
  }
}


/*
 * Send the msg
 * @return true if successful, false otherwise
 */
bool RobeUsbWidget::SendMessage(uint8_t packet_type,
                                const uint8_t *data,
                                unsigned int length) const {
  if (length && !data)
    return false;

  message_header header;
  header.som = SOM;
  header.packet_type = packet_type;
  header.len = length & 0xFF;
  header.len_hi = (length & 0xFF00) >> 8;

  uint8_t crc = SOM + packet_type + (length & 0xFF) + ((length & 0xFF00) >> 8);
  header.header_crc = crc;

  // should really use writev here instead
  ssize_t bytes_sent = m_descriptor->Send(reinterpret_cast<uint8_t*>(&header),
                                          sizeof(header));

  crc += crc;
  if (bytes_sent != sizeof(header))
    // we've probably screwed framing at this point
    return false;

  if (length) {
    unsigned int bytes_sent = m_descriptor->Send(data, length);
    if (bytes_sent != length)
      // we've probably screwed framing at this point
      return false;

    for (unsigned int i = 0; i < length; i++)
      crc += data[i];
  }

  bytes_sent = m_descriptor->Send(&crc, sizeof(crc));
  if (bytes_sent != sizeof(crc))
    return false;
  return true;
}


void RobeUsbWidget::CloseDescriptor() {
  m_descriptor->Close();
}


/*
 * Read the data and handle the messages.
 */
void RobeUsbWidget::ReceiveMessage() {
  unsigned int count;

  switch (m_state) {
    case PRE_SOM:
      do {
        m_descriptor->Receive(&m_header.som, 1, count);
        if (count != 1)
          return;
      } while (m_header.som != SOM);
      m_state = RECV_PACKET_TYPE;
    case RECV_PACKET_TYPE:
      m_descriptor->Receive(&m_header.packet_type, 1, count);
      if (count != 1)
        return;
      m_state = RECV_SIZE_LO;
    case RECV_SIZE_LO:
      m_descriptor->Receive(&m_header.len, 1, count);
      if (count != 1)
        return;
      m_state = RECV_SIZE_HI;
    case RECV_SIZE_HI:
      m_descriptor->Receive(&m_header.len_hi, 1, count);
      if (count != 1)
        return;

      m_data_size = (m_header.len_hi << 8) + m_header.len;
      if (m_data_size > MAX_DATA_SIZE) {
        m_state = PRE_SOM;
        return;
      }

      m_bytes_received = 0;
      m_state = RECV_HEADER_CRC;
    case RECV_HEADER_CRC:
      m_descriptor->Receive(&m_header.header_crc, 1, count);
      if (count != 1)
        return;

      m_crc = SOM + m_header.packet_type + m_header.len + m_header.len_hi;
      if (m_crc != m_header.header_crc) {
        OLA_WARN << "Mismatched header crc: " <<
          static_cast<int>(m_crc) << " != " <<
          static_cast<int>(m_header.header_crc);
        m_state = PRE_SOM;
        return;
      }
      m_crc += m_header.header_crc;

      if (m_data_size)
        m_state = RECV_BODY;
      else
        m_state = RECV_CRC;
    case RECV_BODY:
      m_descriptor->Receive(
          reinterpret_cast<uint8_t*>(&m_recv_buffer) + m_bytes_received,
          m_data_size - m_bytes_received,
          count);

      if (!count)
        return;

      m_bytes_received += count;
      if (m_bytes_received != m_data_size)
        return;

      m_state = RECV_CRC;
    case RECV_CRC:
      // check this is a valid frame
      uint8_t crc;
      m_descriptor->Receive(&crc, 1, count);
      if (count != 1)
        return;

      for (unsigned int i = 0; i < m_data_size; i++)
        m_crc += m_recv_buffer[i];

      if (m_crc != crc) {
        OLA_WARN << "Mismatched header crc: " <<
          std::hex << static_cast<int>(m_crc) << " != " <<
          std::hex << static_cast<int>(crc);
      } else if (m_callback) {
        m_callback->Run(m_header.packet_type,
                        m_data_size ? m_recv_buffer : NULL,
                        m_data_size);
      }
      m_state = PRE_SOM;
  }
  return;
}
}  // usbpro
}  // plugin
}  // ola
