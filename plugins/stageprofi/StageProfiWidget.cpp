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
 * StageprofiWidget.cpp
 * This is the base widget class
 * Copyright (C) 2006 Simon Newton
 */

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends

#include "plugins/stageprofi/StageProfiWidget.h"

#include <stdint.h>
#include <string.h>
#include <algorithm>
#include "ola/Callback.h"
#include "ola/util/Utils.h"

namespace ola {
namespace plugin {
namespace stageprofi {

using ola::io::SelectServer;

enum stageprofi_packet_type_e {
  ID_GETDMX =  0xFE,
  ID_SETDMX = 0xFF,
  ID_SETLO =  0xE0,
  ID_SETHI =  0xE1,
};

typedef enum stageprofi_packet_type_e stageprofi_packet_type;


/*
 * New widget
 */
StageProfiWidget::~StageProfiWidget() {
  if (m_socket) {
    m_socket->Close();
    delete m_socket;
  }
}


/*
 * Disconnect from the widget
 */
int StageProfiWidget::Disconnect() {
  m_socket->Close();
  return 0;
}


/*
 * Send a dmx msg.
 * This has the nasty property of blocking if we remove the device
 * TODO: fix this
 */
bool StageProfiWidget::SendDmx(const DmxBuffer &buffer) const {
  uint16_t index = 0;
  while (index < buffer.Size()) {
    unsigned int size = std::min((unsigned int) DMX_MSG_LEN,
                                 buffer.Size() - index);
    Send255(index, buffer.GetRaw() + index, size);
    index += size;
  }
  return true;
}


/*
 * Called when there is adata to read
 */
void StageProfiWidget::SocketReady() {
  while (m_socket->DataRemaining() > 0) {
    DoRecv();
  }
}


void StageProfiWidget::Timeout() {
  if (m_ss)
    m_ss->Terminate();
}

/*
 * Check if this is actually a StageProfi device
 * @return true if this is a stageprofi,  false otherwise
 */
bool StageProfiWidget::DetectDevice() {
  if (m_ss)
    delete m_ss;

  m_got_response = false;
  m_ss = new SelectServer();
  m_ss->AddReadDescriptor(m_socket, false);
  m_ss->RegisterSingleTimeout(
      100,
      ola::NewSingleCallback(this, &StageProfiWidget::Timeout));

  // try a command, we should get a response
  SetChannel(0, 0);

  m_ss->Run();
  delete m_ss;
  m_ss = NULL;
  if (m_got_response)
    return true;

  m_socket->Close();
  return false;
}


//-----------------------------------------------------------------------------
// Private methods used for communicating with the widget

/*
 * Set a single channel
 */
int StageProfiWidget::SetChannel(uint16_t chan, uint8_t val) const {
  uint8_t msg[3];

  msg[0] = chan > DMX_MSG_LEN ? ID_SETHI : ID_SETLO;
  msg[1] = chan & UINT8_MAX;
  msg[2] = val;
  return m_socket->Send(msg, sizeof(msg));
}


/*
 * Send 255 channels worth of data
 * @param start the start channel for the data
 * @param buf a pointer to the data
 * @param len the length of the data
 */
int StageProfiWidget::Send255(uint16_t start, const uint8_t *buf,
                              unsigned int length) const {
  uint8_t msg[DMX_MSG_LEN + DMX_HEADER_SIZE];
  unsigned int len = std::min((unsigned int) DMX_MSG_LEN, length);

  msg[0] = ID_SETDMX;
  ola::utils::SplitUInt16(start, &msg[2], &msg[1]);
  msg[3] = len;
  memcpy(msg + DMX_HEADER_SIZE, buf, len);
  return m_socket->Send(msg, len + DMX_HEADER_SIZE);
}


/*
 *
 */
int StageProfiWidget::DoRecv() {
  uint8_t byte = 0x00;
  unsigned int data_read;

  while (byte != 'G') {
    int ret = m_socket->Receive(&byte, 1, data_read);

    if (ret == -1 || data_read != 1) {
      return -1;
    }
  }
  m_got_response = true;
  return 0;
}
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
