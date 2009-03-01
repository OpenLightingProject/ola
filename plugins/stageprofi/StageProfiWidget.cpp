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
 * StageProfi Widget
 * Copyright (C) 2006-2007 Simon Newton
 *
 * The device represents the widget.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <llad/logger.h>

#include "StageProfiWidget.h"

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#define min(a,b) a<b?a:b

namespace lla {
namespace plugin {


static const unsigned DMX_MSG_LEN = 255;

enum stageprofi_packet_type_e {
  ID_GETDMX =  0xFE,
  ID_SETDMX = 0xFF,
  ID_SETLO =  0xE0,
  ID_SETHI =  0xE1,
};

typedef enum stageprofi_packet_type_e stageprofi_packet_type;


/*
 *
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
int StageProfiWidget::SendDmx(uint8_t *buf, unsigned int len) const {
  unsigned int start = 0;
  unsigned int l = 0;

  while (start < len) {
    // send the other 256
    l = min(DMX_MSG_LEN, len - start);
    Send255(start, buf + start, l);
    start += l;
  }
  return 0;
}


/*
 * Called when there is adata to read
 */
int StageProfiWidget::SocketReady(ConnectedSocket *socket) {
  while (socket->UnreadData() > 0) {
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
  m_ss->RegisterTimeout(100, this, false);

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
 */
int StageProfiWidget::Send255(unsigned int start, uint8_t *buf, unsigned int len) const {
  uint8_t msg[256 + 4];
  unsigned int l = min(DMX_MSG_LEN, len);

  msg[0] = ID_SETDMX;
  msg[1] = start & 0xFF;
  msg[2] = (start>>8) & 0xFF;
  msg[3] = l;
  memcpy(msg+4, buf, l);
  //printf("send %hhx %hhx %hhx %hhx %hhx %hhx, %hhx %hhx\n", msg[0], msg[1], msg[2], msg[3], msg[4], msg[258], msg[259]);
  return m_socket->Send(msg, len + 4);
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

} // plugin
} //lla
