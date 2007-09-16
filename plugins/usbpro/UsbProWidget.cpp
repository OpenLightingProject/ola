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
 * usbprowidget.cpp
 * UsbPro Widget
 * Copyright (C) 2006  Simon Newton
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

#include <llad/logger.h>

#include "UsbProWidget.h"

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#define min(a,b) a<b?a:b

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
int UsbProWidget::connect(const string &path) {
  struct termios newtio;
  m_fd = open(path.c_str(), O_RDWR | O_NONBLOCK);

  if (m_fd == -1) {
    return 1;
  }

  bzero(&newtio, sizeof(newtio)); // clear struct for new port settings
  tcsetattr(m_fd,TCSANOW,&newtio);

  init();
  return 0;
}


/*
 * Disconnect from the widget
 */
int UsbProWidget::disconnect() {
  if (m_fd)
    close(m_fd);

  m_fd = -1;
  return 0;
}


/*
 * Send a dmx msg
 */
int UsbProWidget::send_dmx(uint8_t *buf, unsigned int len) const {
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
 *
 */
int UsbProWidget::send_rdm(uint8_t *buf, unsigned int len) const {
  promsg msg;
  msg.som = som;
  msg.label = ID_RDM;
  set_msg_len(&msg, len);
  memcpy(&msg.pm_rdm.dmx, buf, len);

  return send_msg(&msg);
}


/*
 * Send a set param request
 *
 * @param usrsz  size of user configurable memory to fetch
 */
int UsbProWidget::set_params(uint8_t *data, unsigned int len, uint8_t brk, uint8_t mab, uint8_t rate) {
  int l = min( (unsigned int) USER_CONFIG_LEN, len);
  promsg msg;
  msg.som = som;
  msg.label = ID_PRMSET;
  set_msg_len(&msg, sizeof(pms_prmset) - USER_CONFIG_LEN + l);
  msg.pm_prmset.len = len & 0xFF;
  msg.pm_prmset.len_hi = (len & 0xFF) >> 8;
  msg.pm_prmset.brk = brk;
  msg.pm_prmset.mab = mab;
  msg.pm_prmset.rate = rate;
  memcpy(msg.pm_prmset.user, data, l);
  send_msg(&msg);

  // send a param request to get the new values
  send_prmreq(0);
  return 0;
}


/*
 * Fetch the widget parameters
 */
void UsbProWidget::get_params(uint16_t *firmware, uint8_t *brk, uint8_t *mab, uint8_t *rate) const {
  *firmware = m_params.firmv_hi << 8 | m_params.firmv;
  *brk = m_params.brtm;
  *mab = m_params.mabtm;
  *rate = m_params.rate;
}


/*
 * Fetch the serial number
 */
void UsbProWidget::get_serial(uint8_t *serial, unsigned int len) const {
  unsigned int l = min(len, sizeof(pms_snorep));
  memcpy(serial, m_serial, l);
}


/*
 * get the dmx
 */
int UsbProWidget::get_dmx(uint8_t *data, unsigned int len) {
  int l = min(len, DMX_BUF_LEN-1);

  // byte 0 is the start code which we ignore
  memcpy(data,m_dmx+1, l);
  return l;
}


/*
 * Force the widget back into receiving mode
 */
int UsbProWidget::recv_mode() {
  return send_rcmode(1);

}


/*
 * read data from the widget
 */
int UsbProWidget::recv() {
  int unread;

  // check if there is data to be read
  if (ioctl( m_fd, FIONREAD , &unread)) {
    // the device has been removed
    Logger::instance()->log(Logger::WARN, "UsbProPlugin: device removed" );
    return -1;
  }

  while (unread != 0) {
    do_recv();
    ioctl(m_fd, FIONREAD, &unread);
  }

  return 0;
}


/*
 * Set the object to be notified when the dmx changes
 */
void UsbProWidget::set_listener(UsbProWidgetListener *l) {
  m_listener = l;
}


//-----------------------------------------------------------------------------
// Private methods used for communicating with the widget

/*
 * Send a get param request
 *
 * @param usrsz  size of user configurable memory to fetch
 */
int UsbProWidget::send_prmreq(int usrsz) const {
  promsg msg;
  msg.som = som;
  msg.label = ID_PRMREQ;
  set_msg_len(&msg, sizeof(pms_prmreq));
  msg.pm_prmreq.len = usrsz & 0xFF;
  msg.pm_prmreq.len_hi = (usrsz & 0xFF) >> 8;
  return send_msg(&msg);
}


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
 * Send a serial number request
 */
int UsbProWidget::send_snoreq() const {
  promsg msg;
  msg.som = som;
  msg.label = ID_SNOREQ;
  set_msg_len(&msg, sizeof(pms_snoreq));

  return send_msg(&msg);
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

    if ( cos->changed[i/8] & (1 << (i%8)) ) {
      m_dmx[chn_st + i] = cos->data[offset];
      offset++;
    }
  }

  if (m_listener != NULL)
    m_listener->new_dmx();

  return 0;
}


/*
 * Handle the param reply
 *
 * @param rep parameters message
 * @param len length of the message
 */
int UsbProWidget::handle_prmrep(pms_prmrep *rep, unsigned int len) {

  if ( len >= 5 && len <= sizeof(pms_prmrep) ) {
    memcpy(&m_params, rep, sizeof(pms_prmrep));
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

  if ( len == sizeof(pms_snorep) ) {
    memcpy(m_serial , rep->srno, 4);
  }
  return 0;
}

/*
 * Get the serial number and params so we can cache them
 * TODO: This sleeping is ok while the plugins are loaded on startup, but
 * we can't afford to be sleeping once we are running. We may have to move this
 * to a separate thread.
 */
int UsbProWidget::init() {
  struct timespec tv;

  tv.tv_sec = 0;
  tv.tv_nsec = 1000000;

  // set a serial number and params request
  send_prmreq(0);
  nanosleep(&tv, NULL);
  send_snoreq();

  // put us into receiving mode
  send_rcmode(1);

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
  write(m_fd, msg, len+4);
  write(m_fd, &eom, sizeof(eom));
  return 0;
}


/*
 *
 */
int UsbProWidget::do_recv() {
  uint8_t byte = 0x00;
    uint8_t label;
    int cnt, plen, bytes_read;
    pmu buf;

    while(byte != 0x7e) {
    cnt = read(m_fd,&byte,1);

    if (cnt != 1) {
      printf("1, read to much %i\n", cnt);
      return -1;
    }
  }

    // try to read the label
    cnt = read(m_fd,&label,1);

    if (cnt != 1) {
    printf("2, could not read label %i\n", cnt);
    return 1;
    }

    cnt = read(m_fd, &byte,1);
    if (cnt != 1) {
    printf("3, could not read len hi%i\n", cnt);
    return 1;
    }
    plen = byte;

    cnt = read(m_fd, &byte,1);
    if (cnt != 1) {
        printf("4, could not read len lo %i\n", cnt);
    return 1;
    }
    plen += byte<<8;

    for (bytes_read = 0; bytes_read < plen;) {
    bytes_read += read(m_fd, (uint8_t*) &buf +bytes_read, plen-bytes_read);
    }

    // check this is a valid frame with an end byte
    cnt = read(m_fd, &byte,1);
    if (cnt != 1) {
        printf("5, read to much %i\n", cnt);
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
        printf("not sure what msg this is\n");
    }
  }
  return 0;
}
