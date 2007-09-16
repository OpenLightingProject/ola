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
 * stageprofiwidget.cpp
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

static const unsigned DMX_MSG_LEN = 255;

enum stageprofi_packet_type_e {
  ID_GETDMX =  0xFE,
  ID_SETDMX = 0xFF,
  ID_SETLO =  0xE0,
  ID_SETHI =  0xE1,
};

typedef enum stageprofi_packet_type_e stageprofi_packet_type;


/*
 * Disconnect from the widget
 */
int StageProfiWidget::disconnect() {
  if (m_fd)
    close(m_fd);

  m_fd = -1;
  return 0;
}


/*
 * Send a dmx msg.
 * This has the nasty property of blocking if we remove the device
 * TODO: fix this
 */
int StageProfiWidget::send_dmx(uint8_t *buf, unsigned int len) const {
  unsigned int start = 0;
  unsigned int l = 0;

  while (start < len) {
    // send the other 256
    l = min(DMX_MSG_LEN, len-start);
    send_255(start, buf+start, l);
    start += l;
  }

  return 0;
}


/*
 * read data from the widget
 */
int StageProfiWidget::recv() {
  int unread;

  // check if there is data to be read
  if ( ioctl( m_fd, FIONREAD , &unread) ) {
    // the device has been removed
    Logger::instance()->log(Logger::WARN, "StageProfiPlugin: device removed");
    return -1;
  }

  while (unread != 0) {
    do_recv();
    ioctl(m_fd, FIONREAD, &unread);
  }

  return 0;
}

/*
 * Check if this is actually a StageProfi device
 * @return zero if this is a stageprofi, false otherwise
 */
int StageProfiWidget::detect_device() const {
  uint8_t byte = 0x00;
  fd_set r_fds;
  struct timeval tv;
  int maxsd, ret;

  // try a command, we should get a response
  set_channel(0,0);

  FD_ZERO(&r_fds);
  FD_SET(m_fd, &r_fds);
  maxsd = m_fd + 1;

  // wait 100ms for a response
  tv.tv_sec = 0;
  tv.tv_usec = 100000;

  while (1) {
    switch( select(maxsd+1, &r_fds, NULL, NULL, &tv)) {
      case 0:
        // timeout
        goto e_close;
        break;
      case -1:
        goto e_close;
        break;
      default:
        ret = read(m_fd, &byte, 1);
        if ( ! ret || byte != 'G') {
          goto e_close;
        }
        return 0;
    }
  }

e_close:
    close(m_fd);
    return -1;
}




//-----------------------------------------------------------------------------
// Private methods used for communicating with the widget

/*
 * Set a single channel
 */
int StageProfiWidget::set_channel(unsigned int chan, uint8_t val) const {
  uint8_t msg[3];

  msg[0] = chan > DMX_MSG_LEN ? ID_SETHI : ID_SETLO;
  msg[1] = chan & 0xFF;
  msg[2] = val;
  return write(m_fd, msg, sizeof(msg));
}


/*
 * Send 255 channels worth of data
 */
int StageProfiWidget::send_255(unsigned int start, uint8_t *buf, unsigned int len) const {
  uint8_t msg[256+4];
  unsigned int l = min(DMX_MSG_LEN, len);

  msg[0] = ID_SETDMX;
  msg[1] = start & 0xFF;
  msg[2] = (start>>8) & 0xFF;
  msg[3] = l;
  memcpy(msg+4, buf, l);
  //printf("send %hhx %hhx %hhx %hhx %hhx %hhx, %hhx %hhx\n", msg[0], msg[1], msg[2], msg[3], msg[4], msg[258], msg[259]);
  return write(m_fd, msg, len+4);
}


/*
 *
 */
int StageProfiWidget::do_recv() {
  uint8_t byte = 0x00;
  int cnt;

  while (byte != 'G') {
    cnt = read(m_fd, &byte, 1);

    if (cnt != 1) {
      return -1;
    }
  }
  return 0;
}
