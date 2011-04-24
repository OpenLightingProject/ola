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
 * UsbWidget.cpp
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
#include "ola/Logging.h"
#include "plugins/usbpro/UsbWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {


UsbWidget::UsbWidget(ola::network::ConnectedSocket *socket)
    : m_callback(NULL),
      m_socket(socket),
      m_state(PRE_SOM),
      m_bytes_received(0) {
  m_socket->SetOnData(NewCallback(this, &UsbWidget::SocketReady));
}


UsbWidget::~UsbWidget() {
  if (m_callback)
    delete m_callback;
  // don't delete because ownership is transferred to the ss so that device
  // removal works correctly. If you delete the socket the OnClose closure will
  // be deleted, which breaks if it's already running.
  m_socket->SetOnClose(NULL);
  m_socket->Close();
  m_socket = NULL;
}


/**
 * Set the closure to be called when we receive a message from the widget
 * @param callback the closure to run, ownership is transferred.
 */
void UsbWidget::SetMessageHandler(
    ola::Callback3<void, uint8_t, const uint8_t*, unsigned int> *callback) {
  if (m_callback)
    delete m_callback;
  m_callback = callback;
}


/*
 * Set the onRemove handler
 */
void UsbWidget::SetOnRemove(ola::SingleUseCallback0<void> *on_close) {
  m_socket->SetOnClose(on_close);
}


/*
 * Read data from the widget
 */
void UsbWidget::SocketReady() {
  while (m_socket->DataRemaining() > 0) {
    ReceiveMessage();
  }
}


/*
 * Send the msg
 * @return true if successful, false otherwise
 */
bool UsbWidget::SendMessage(uint8_t label,
                            const uint8_t *data,
                            unsigned int length) const {
  if (length && !data)
    return false;

  message_header header;
  header.som = SOM;
  header.label = label;
  header.len = length & 0xFF;
  header.len_hi = (length & 0xFF00) >> 8;

  // should really use writev here instead
  ssize_t bytes_sent = m_socket->Send(reinterpret_cast<uint8_t*>(&header),
                                      sizeof(header));
  if (bytes_sent != sizeof(header))
    // we've probably screwed framing at this point
    return false;

  if (length) {
    unsigned int bytes_sent = m_socket->Send(data, length);
    if (bytes_sent != length)
      // we've probably screwed framing at this point
      return false;
  }

  uint8_t eom = EOM;
  bytes_sent = m_socket->Send(&eom, sizeof(EOM));
  if (bytes_sent != sizeof(EOM))
    return false;
  return true;
}


void UsbWidget::CloseSocket() {
  m_socket->Close();
}


/**
 * Open a path and apply the settings required for talking to widgets.
 */
ola::network::ConnectedSocket *UsbWidget::OpenDevice(
    const string &path) {
  struct termios newtio;
  int fd = open(path.data(), O_RDWR | O_NONBLOCK | O_NOCTTY);

  if (fd == -1) {
    OLA_WARN << "Failed to open " << path << " " << strerror(errno);
    return NULL;
  }

  bzero(&newtio, sizeof(newtio));  // clear struct for new port settings
  cfsetispeed(&newtio, B115200);
  cfsetospeed(&newtio, B115200);
  tcsetattr(fd, TCSANOW, &newtio);

  return new ola::network::DeviceSocket(fd);
}


/*
 * Read the data and handle the messages.
 */
void UsbWidget::ReceiveMessage() {
  unsigned int count, packet_length;

  switch (m_state) {
    case PRE_SOM:
      do {
        m_socket->Receive(&m_header.som, 1, count);
        if (count != 1)
          return;
      } while (m_header.som != SOM);
      m_state = RECV_LABEL;
    case RECV_LABEL:
      m_socket->Receive(&m_header.label, 1, count);
      if (count != 1)
        return;
      m_state = RECV_SIZE_LO;
    case RECV_SIZE_LO:
      m_socket->Receive(&m_header.len, 1, count);
      if (count != 1)
        return;
      m_state = RECV_SIZE_HI;
    case RECV_SIZE_HI:
      m_socket->Receive(&m_header.len_hi, 1, count);
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
    case RECV_BODY:
      packet_length = (m_header.len_hi << 8) + m_header.len;
      m_socket->Receive(
          reinterpret_cast<uint8_t*>(&m_recv_buffer) + m_bytes_received,
          packet_length - m_bytes_received,
          count);

      if (!count)
        return;

      m_bytes_received += count;
      if (m_bytes_received != packet_length)
        return;

      m_state = RECV_EOM;
    case RECV_EOM:
      // check this is a valid frame with an end byte
      uint8_t eom;
      m_socket->Receive(&eom, 1, count);
      if (count != 1)
        return;

      packet_length = (m_header.len_hi << 8) + m_header.len;

      if (eom == EOM && m_callback)
        m_callback->Run(m_header.label,
                        packet_length ? m_recv_buffer : NULL,
                        packet_length);
      m_state = PRE_SOM;
  }
  return;
}
}  // usbpro
}  // plugin
}  // ola
