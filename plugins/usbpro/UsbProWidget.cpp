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
 * UsbProWidget.cpp
 * UsbPro Widget
 * Copyright (C) 2006-2008 Simon Newton
 *
 * The device represents the widget.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <lla/Logging.h>

#include "UsbProWidget.h"

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#define min(a,b) a<b?a:b

namespace lla {
namespace plugin {

using std::string;

static const uint8_t RCMODE_ALWAYS = 0x00;
static const uint8_t RCMODE_CHANGE = 0x01;

static const uint8_t som = 0x7e;
static const uint8_t eom = 0xe7;

enum usbpro_packet_type_e {
  ID_PRMREQ = 0x03,
  ID_PRMREP =  0x03,
  ID_PRMSET = 0x04,
  ID_RDMX  =  0x05,
  ID_SDMX  =  0x06,
  ID_RDM =   0x07,
  ID_RCMODE =  0x08,
  ID_COS =  0x09,
  ID_SNOREQ =  0x0A,
  ID_SNOREP =  0x0A
};

typedef enum usbpro_packet_type_e usbpro_packet_type;

/*
 * Connect to the widget
 */
int UsbProWidget::Connect(const string &path) {
  if (m_enabled)
    return -1;

  if (m_socket) {
    m_socket->Close();
    delete m_socket;
  }

  struct termios newtio;
  int fd = open(path.c_str(), O_RDWR | O_NONBLOCK | O_NOCTTY);

  if (fd == -1) {
    return 1;
  }

  bzero(&newtio, sizeof(newtio)); // clear struct for new port settings
  m_socket = new ConnectedSocket(fd, fd);
  m_socket->SetListener(this);
  m_enabled = true;

  // fire off a get request
  GetParameters();

  // put us into receiving mode
  send_rcmode(1);
  return 0;
}


/*
 * Disconnect from the widget
 */
int UsbProWidget::Disconnect() {
  if (!m_enabled)
    return 0;

  if (m_socket) {
    m_socket->Close();
    delete m_socket;
    m_socket = NULL;
  }
  m_enabled = false;
  return 0;
}


/*
 * Send a dmx msg
 */
int UsbProWidget::SendDmx(uint8_t *buf, unsigned int len) const {
  unsigned int l = min((unsigned int) DMX_BUF_LEN, len);
  promsg msg;

  msg.som = som;
  msg.label = ID_SDMX;
  set_msg_len(&msg, l+1);

  //start code to 0
  msg.pm_dmx.dmx[0] = 0;
  memcpy(&msg.pm_dmx.dmx[1], buf, l);

  return send_msg(&msg);
}


/*
 * Send a rdm msg, rdm support is a bit sucky
 */
int UsbProWidget::SendRdm(uint8_t *buf, unsigned int len) const {
  promsg msg;
  msg.som = som;
  msg.label = ID_RDM;
  set_msg_len(&msg, len);
  memcpy(&msg.pm_rdm.dmx, buf, len);

  return send_msg(&msg);
}


/*
 * Send a set param request. If we haven't received a response to a GetParam()
 * request, the default values won't be known.
 * @param data pointer to the user configurable data
 * @param len  size of user configurable memory to fetch
 * @param brk the break_time or K_MISSING_PARAM to leave it the same
 * @param mab the mab_time or K_MISSING_PARAM to leave it the same
 * @param rate the rate or K_MISSING_PARAM to leave it the same
 * @returns true if the set was sent correctly, false if the default values
 * aren't know.
 */
bool UsbProWidget::SetParameters(uint8_t *data,
                                 unsigned int len,
                                 int brk,
                                 int mab,
                                 int rate) {
  int l = min((unsigned int) USER_CONFIG_LEN, len);
  promsg msg;
  msg.som = som;
  msg.label = ID_PRMSET;
  set_msg_len(&msg, sizeof(pms_prmset) - USER_CONFIG_LEN + l);
  msg.pm_prmset.len = len & 0xFF;
  msg.pm_prmset.len_hi = (len & 0xFF) >> 8;
  brk = brk != K_MISSING_PARAM ? brk : m_break_time;
  mab = mab != K_MISSING_PARAM ? mab : m_mab_time;
  rate = rate != K_MISSING_PARAM ? rate : m_rate;

  if (brk == K_MISSING_PARAM ||
      mab == K_MISSING_PARAM ||
      rate == K_MISSING_PARAM) {
    LLA_WARN << "Missing default values for usb SetParam";
    return false;
  }
  msg.pm_prmset.brk = brk;
  msg.pm_prmset.mab = mab;
  msg.pm_prmset.rate = rate;
  memcpy(msg.pm_prmset.user, data, l);
  return send_msg(&msg);
}


/*
 * Fetch the widget parameters, causes listener->Parameters() to be called
 * later.
 */
bool UsbProWidget::GetParameters() {
  int user_size = 0;
  promsg msg;
  msg.som = som;
  msg.label = ID_PRMREQ;
  set_msg_len(&msg, sizeof(pms_prmreq));
  msg.pm_prmreq.len = user_size & 0xFF;
  msg.pm_prmreq.len_hi = (user_size & 0xFF) >> 8;
  return send_msg(&msg);
}


/*
 * Fetch the serial number, causes listener->SerialNumber() to be called
 * sometime later.
 */
bool UsbProWidget::GetSerial() {
  promsg msg;
  msg.som = som;
  msg.label = ID_SNOREQ;
  set_msg_len(&msg, sizeof(pms_snoreq));
  return send_msg(&msg);
}


/*
 * Copy the latest DMX data into the specified buffer
 * @param data a pointer to the dmx data buffer
 * @param len the length of the dmx data buffer
 */
int UsbProWidget::FetchDmx(uint8_t *data, unsigned int len) {
  int l = min(len, DMX_BUF_LEN - 1);

  // byte 0 is the start code which we ignore
  memcpy(data, m_dmx + 1, l);
  return l;
}


/*
 * Force the widget back into receiving mode
 */
int UsbProWidget::ChangeToReceiveMode() {
  return send_rcmode(1);
}


/*
 * Read data from the widget
 */
int UsbProWidget::SocketReady(ConnectedSocket *socket) {
  while (socket->UnreadData() > 0) {
    do_recv();
  }
  return 0;
}


/*
 * Set the object to be notified when the dmx changes
 */
void UsbProWidget::SetListener(UsbProWidgetListener *listener) {
  m_listener = listener;
}


//-----------------------------------------------------------------------------
// Private methods used for communicating with the widget


/*
 * Send a mode msg.
 * @param mode the mode to change to
 */
int UsbProWidget::send_rcmode(int mode) {
  int ret;
  uint8_t m = mode ? RCMODE_CHANGE : RCMODE_ALWAYS;
  promsg msg;

  msg.som = som;
  msg.label = ID_RCMODE;
  set_msg_len(&msg, sizeof(pms_rcmode));
  msg.pm_rcmode.mode = mode;

  ret = send_msg(&msg);

  if (!ret && m)
    memset(m_dmx, 0x00, DMX_BUF_LEN);
  return ret;
}


/*
 * Handle the dmx frame
 * Don't do anything as we expect cos messages instead
 */
int UsbProWidget::handle_dmx(pms_rdmx *dmx, int len) {
  dmx = NULL;
  len = 0;
//  printf(" stc %hhx  %hhx %hhx\n", dmx->dmx[0], dmx->dmx[1], dmx->dmx[2]);
  return 0;
}


/*
 * Handle the dmx change of state frame
 */
int UsbProWidget::handle_cos(pms_cos *cos, int len) {
  int chn_st = cos->start *8;
  int i, offset;

  // should be checking length here
  offset = 0;
  for (i = 0; i< 40; i++) {

    if (chn_st+i > DMX_BUF_LEN-1 || offset + 6 >= len)
      break;

    if (cos->changed[i/8] & (1 << (i % 8)) ) {
      m_dmx[chn_st + i] = cos->data[offset];
      offset++;
    }
  }

  if (m_listener)
    m_listener->HandleWidgetDmx();

  return 0;
}


/*
 * Handle the param reply
 *
 * @param rep parameters message
 * @param len length of the message
 */
int UsbProWidget::handle_prmrep(pms_prmrep *reply, unsigned int len) {
  // snoop the values to update our cache
  m_break_time = reply->base_parameters.brtm;
  m_mab_time = reply->base_parameters.mabtm;
  m_rate = reply->base_parameters.rate;

  if (m_listener && len >= sizeof(pms_parameters)) {
    m_listener->HandleWidgetParameters(reply->base_parameters.firmv,
                                       reply->base_parameters.firmv_hi,
                                       reply->base_parameters.brtm,
                                       reply->base_parameters.mabtm,
                                       reply->base_parameters.rate);
  }
  return 0;
}


/*
 * Handle the serial number reply
 *
 * @param rep serial number message
 * @param len length of the message
 */
int UsbProWidget::handle_snorep(pms_snorep *rep, int len) {
  if (m_listener && len == sizeof(pms_snorep))
    m_listener->HandleWidgetSerial(rep->srno);
  return 0;
}


/*
 * Set the length of the msg
 */
int UsbProWidget::set_msg_len(promsg *msg, int len) const {
  msg->len = len & 0xFF;
  msg->len_hi = (len & 0xFF00) >> 8;
  return 0;
}


/*
 * Send the msg
 */
int UsbProWidget::send_msg(promsg *msg) const {
  int len = (msg->len_hi << 8) + msg->len;
  unsigned int bytes_sent = 0;
  bytes_sent += m_socket->Send((uint8_t*) msg, len + 4);
  bytes_sent += m_socket->Send(&eom, sizeof(eom));
  return (bytes_sent == len + 4 + sizeof(eom));
}


/*
 *
 */
int UsbProWidget::do_recv() {
  uint8_t byte = 0x00;
  uint8_t label;
  int plen, bytes_read;
  unsigned int cnt;
  pmu buf;

  while(byte != 0x7e) {
    m_socket->Receive((uint8_t*) &byte, 1, cnt);

    if (cnt != 1) {
      LLA_WARN << "read to much, expected 1, got " << cnt;
      return -1;
    }
  }

  // try to read the label
  m_socket->Receive((uint8_t*) &label, 1, cnt);

  if (cnt != 1) {
    LLA_WARN << "could not read label, expected 1, got " << cnt;
    return 1;
  }

  m_socket->Receive((uint8_t*) &byte, 1, cnt);
  if (cnt != 1) {
    LLA_WARN << "could not read len hi, expected 1, got " << cnt;
    return 1;
  }
  plen = byte;

  m_socket->Receive((uint8_t*) &byte, 1, cnt);
  if (cnt != 1) {
    LLA_WARN << "could not read len lo, expected 1, got " << cnt;
    return 1;
  }
  plen += byte << 8;

  for (bytes_read = 0; bytes_read < plen;) {
    m_socket->Receive((uint8_t*) &buf + bytes_read, plen-bytes_read, cnt);
    bytes_read += cnt;
  }

  // check this is a valid frame with an end byte
  m_socket->Receive((uint8_t*) &byte, 1, cnt);
  if (cnt != 1) {
    LLA_WARN << "read to much, expected 1, got " << cnt;
    return 1;
  }

  if (byte == 0xe7) {
    switch(label) {
      case ID_RDMX:
        handle_dmx( &buf.pmu_rdmx, plen);
        break;
      case ID_PRMREP:
        handle_prmrep( &buf.pmu_prmrep, plen);
        break;
      case ID_COS:
        handle_cos( &buf.pmu_cos, plen);
        break;
      case ID_SNOREP:
        handle_snorep( &buf.pmu_snorep, plen);
        break;
      default:
        LLA_WARN << "Unknown message type " << label;
    }
  }
  return 0;
}

} // plugin
} //lla
