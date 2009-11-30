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
 * StageprofiWidget.cpp
 * This is the base widget class
 * Copyright (C) 2006-2009 Simon Newton
 */

#include <string.h>
#include <algorithm>
#include "ola/Closure.h"
#include "plugins/stageprofi/StageProfiWidget.h"

namespace ola {
namespace plugin {
namespace stageprofi {


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
  unsigned int index = 0;
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
int StageProfiWidget::SocketReady() {
  while (m_socket->DataRemaining() > 0) {
    DoRecv();
  }
  return 0;
}


int StageProfiWidget::Timeout() {
  if (m_ss)
    m_ss->Terminate();
  return 0;
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
  m_ss->AddSocket(m_socket, NULL);
  m_ss->RegisterSingleTimeout(
      100,
      ola::NewSingleClosure(this, &StageProfiWidget::Timeout));

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
int StageProfiWidget::SetChannel(unsigned int chan, uint8_t val) const {
  uint8_t msg[3];

  msg[0] = chan > DMX_MSG_LEN ? ID_SETHI : ID_SETLO;
  msg[1] = chan & 0xFF;
  msg[2] = val;
  return m_socket->Send(msg, sizeof(msg));
}


/*
 * Send 255 channels worth of data
 * @param start the start channel for the data
 * @param buf a pointer to the data
 * @param len the length of the data
 */
int StageProfiWidget::Send255(unsigned int start, const uint8_t *buf,
                              unsigned int length) const {
  uint8_t msg[DMX_MSG_LEN + DMX_HEADER_SIZE];
  unsigned int len = std::min((unsigned int) DMX_MSG_LEN, length);

  msg[0] = ID_SETDMX;
  msg[1] = start & 0xFF;
  msg[2] = (start>>8) & 0xFF;
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
}  // stageprofi
}  // plugin
}  // ola
