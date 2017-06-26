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
 * BaseRobeWidget.cpp
 * Read and Write to a Robe USB Widget.
 * Copyright (C) 2011 Simon Newton
 */

#include <string.h>
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/base/Macro.h"
#include "plugins/usbpro/BaseRobeWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

const unsigned int BaseRobeWidget::HEADER_SIZE =
  sizeof(BaseRobeWidget::message_header);

const uint8_t BaseRobeWidget::CHANNEL_A_OUT;
const uint8_t BaseRobeWidget::INFO_REQUEST;
const uint8_t BaseRobeWidget::INFO_RESPONSE;
const uint8_t BaseRobeWidget::RDM_DISCOVERY;
const uint8_t BaseRobeWidget::RDM_DISCOVERY_RESPONSE;
const uint8_t BaseRobeWidget::RDM_REQUEST;
const uint8_t BaseRobeWidget::RDM_RESPONSE;
const uint8_t BaseRobeWidget::UID_REQUEST;
const uint8_t BaseRobeWidget::UID_RESPONSE;
const uint8_t BaseRobeWidget::DMX_IN_REQUEST;
const uint8_t BaseRobeWidget::DMX_IN_RESPONSE;
const uint8_t BaseRobeWidget::SOM;

BaseRobeWidget::BaseRobeWidget(ola::io::ConnectedDescriptor *descriptor)
    : m_descriptor(descriptor),
      m_state(PRE_SOM),
      m_bytes_received(0),
      m_data_size(0),
      m_crc(0) {
  memset(&m_header, 0, sizeof(m_header));
  m_descriptor->SetOnData(NewCallback(this, &BaseRobeWidget::DescriptorReady));
}


BaseRobeWidget::~BaseRobeWidget() {
  m_descriptor->SetOnData(NULL);
}


/*
 * Send the msg
 * @return true if successful, false otherwise
 */
bool BaseRobeWidget::SendMessage(uint8_t packet_type,
                                 const uint8_t *data,
                                 unsigned int length) const {
  if (length && !data)
    return false;

  ssize_t frame_size = HEADER_SIZE + length + 1;
  uint8_t frame[frame_size];
  message_header *header = reinterpret_cast<message_header*>(frame);
  header->som = SOM;
  header->packet_type = packet_type;
  header->len = length & 0xFF;
  header->len_hi = (length & 0xFF00) >> 8;
  uint8_t crc = SOM + packet_type + (length & 0xFF) + ((length & 0xFF00) >> 8);
  header->header_crc = crc;
  crc += crc;
  for (unsigned int i = 0; i < length; i++)
    crc += data[i];

  memcpy(frame + sizeof(message_header), data, length);
  frame[frame_size - 1] = crc;

  ssize_t bytes_sent = m_descriptor->Send(frame, frame_size);
  if (bytes_sent != frame_size)
    // we've probably screwed framing at this point
    return false;

  return true;
}


/*
 * Read data from the widget
 */
void BaseRobeWidget::DescriptorReady() {
  while (m_descriptor->DataRemaining() > 0) {
    ReceiveMessage();
  }
}


/*
 * Read the data and handle the messages.
 */
void BaseRobeWidget::ReceiveMessage() {
  unsigned int count;

  switch (m_state) {
    case PRE_SOM:
      do {
        m_descriptor->Receive(&m_header.som, 1, count);
        if (count != 1)
          return;
      } while (m_header.som != SOM);
      m_state = RECV_PACKET_TYPE;

      // if we don't return, fallthrough
      OLA_FALLTHROUGH
    case RECV_PACKET_TYPE:
      m_descriptor->Receive(&m_header.packet_type, 1, count);
      if (count != 1)
        return;
      m_state = RECV_SIZE_LO;

      // if we don't return, fallthrough
      OLA_FALLTHROUGH
    case RECV_SIZE_LO:
      m_descriptor->Receive(&m_header.len, 1, count);
      if (count != 1)
        return;
      m_state = RECV_SIZE_HI;

      // if we don't return, fallthrough
      OLA_FALLTHROUGH
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

      // if we don't return, fallthrough
      OLA_FALLTHROUGH
    case RECV_HEADER_CRC:
      m_descriptor->Receive(&m_header.header_crc, 1, count);
      if (count != 1)
        return;

      m_crc = SOM + m_header.packet_type + m_header.len + m_header.len_hi;
      if (m_crc != m_header.header_crc) {
        OLA_WARN << "Mismatched header crc: " << std::hex <<
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

      // if we don't return, fallthrough
      OLA_FALLTHROUGH
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

      // if we don't return, fallthrough
      OLA_FALLTHROUGH
    case RECV_CRC:
      // check this is a valid frame
      uint8_t crc;
      m_descriptor->Receive(&crc, 1, count);
      if (count != 1)
        return;

      for (unsigned int i = 0; i < m_data_size; i++)
        m_crc += m_recv_buffer[i];

      if (m_crc != crc) {
        OLA_WARN << "Mismatched data crc: " <<
          std::hex << static_cast<int>(m_crc) << " != " <<
          std::hex << static_cast<int>(crc);
      } else {
        HandleMessage(m_header.packet_type,
                      m_data_size ? m_recv_buffer : NULL,
                      m_data_size);
      }
      m_state = PRE_SOM;

      // if we don't return, fallthrough
      OLA_FALLTHROUGH
  }
  return;
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
