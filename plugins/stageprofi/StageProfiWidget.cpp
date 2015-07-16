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
 * StageProfiWidget.cpp
 * This is the base widget class
 * Copyright (C) 2006 Simon Newton
 */

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends

#include "plugins/stageprofi/StageProfiWidget.h"

#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <string>
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/util/Utils.h"

namespace ola {
namespace plugin {
namespace stageprofi {

using ola::io::ConnectedDescriptor;
using ola::TimeInterval;
using ola::thread::INVALID_TIMEOUT;
using std::string;

enum stageprofi_packet_type_e {
  ID_GETDMX =  0xFE,
  ID_SETDMX = 0xFF,
  ID_SETLO =  0xE0,
  ID_SETHI =  0xE1,
};

typedef enum stageprofi_packet_type_e stageprofi_packet_type;


StageProfiWidget::StageProfiWidget(io::SelectServerInterface *ss,
                                   ConnectedDescriptor *descriptor,
                                   const string &widget_path,
                                   DisconnectCallback *disconnect_cb)
    : m_ss(ss),
      m_descriptor(descriptor),
      m_widget_path(widget_path),
      m_disconnect_cb(disconnect_cb),
      m_timeout_id(INVALID_TIMEOUT),
      m_got_response(false) {
  m_descriptor->SetOnData(
      NewCallback<StageProfiWidget>(this, &StageProfiWidget::SocketReady));
  m_ss->AddReadDescriptor(m_descriptor.get());
  m_timeout_id = m_ss->RegisterSingleTimeout(
      TimeInterval(1, 0),
      ola::NewSingleCallback(this, &StageProfiWidget::DiscoveryTimeout));
  SendQueryPacket();
}

StageProfiWidget::~StageProfiWidget() {
  if (m_timeout_id != INVALID_TIMEOUT) {
    m_ss->RemoveTimeout(m_timeout_id);
  }

  if (m_descriptor.get()) {
    m_ss->RemoveReadDescriptor(m_descriptor.get());
  }

  if (m_disconnect_cb) {
    delete m_disconnect_cb;
  }
}

bool StageProfiWidget::SendDmx(const DmxBuffer &buffer) {
  if (!m_got_response) {
    return false;
  }

  uint16_t index = 0;
  while (index < buffer.Size()) {
    unsigned int size = std::min((unsigned int) DMX_MSG_LEN,
                                 buffer.Size() - index);
    bool ok = Send255(index, buffer.GetRaw() + index, size);
    if (!ok) {
      OLA_INFO << "Failed to send StageProfi message, closing socket";
      RunDisconnectHandler();
    }
    index += size;
  }
  return true;
}

/*
 * Called when there is data to read.
 */
void StageProfiWidget::SocketReady() {
  while (m_descriptor->DataRemaining() > 0) {
    uint8_t byte = 0x00;
    unsigned int data_read;
    while (byte != 'G') {
      int ret = m_descriptor->Receive(&byte, 1, data_read);

      if (ret == -1 || data_read != 1) {
        return;
      }
    }
    m_got_response = true;
  }
}

void StageProfiWidget::DiscoveryTimeout() {
  if (!m_got_response) {
    OLA_INFO << "No response from StageProfiWidget";
    RunDisconnectHandler();
  }
}

/*
 * @brief Send 255 channels worth of data
 * @param start the start channel for the data
 * @param buf a pointer to the data
 * @param len the length of the data
 */
bool StageProfiWidget::Send255(uint16_t start,
                               const uint8_t *buf,
                               unsigned int length) const {
  uint8_t msg[DMX_MSG_LEN + DMX_HEADER_SIZE];
  unsigned int len = std::min((unsigned int) DMX_MSG_LEN, length);

  msg[0] = ID_SETDMX;
  ola::utils::SplitUInt16(start, &msg[2], &msg[1]);
  msg[3] = len;
  memcpy(msg + DMX_HEADER_SIZE, buf, len);

  // This needs to be an int, as m_descriptor->Send() below returns an int
  const int bytes_to_send = len + DMX_HEADER_SIZE;
  return m_descriptor->Send(msg, bytes_to_send) == bytes_to_send;
}

void StageProfiWidget::SendQueryPacket() {
  uint8_t query[] = {'C', '?'};
  ssize_t bytes_sent = m_descriptor->Send(query, arraysize(query));
  OLA_DEBUG << "Sending StageprofiWidget query: C? returned " << bytes_sent;
}

void StageProfiWidget::RunDisconnectHandler() {
  if (m_disconnect_cb) {
    m_disconnect_cb->Run();
    m_disconnect_cb = NULL;
  }
}
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
