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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <ola/Logging.h>

#include "UsbProWidget.h"

namespace ola {
namespace usbpro {

using std::string;

const char UsbProWidget::REPLY_SUCCESS[] = "TRUE";

typedef enum {
  ID_PROGRAM_FIRMWARE = 0x01,
  ID_FLASH_PAGE = 0x02,
  ID_PRMREQ = 0x03,
  ID_PRMREP = 0x03,
  ID_PRMSET = 0x04,
  ID_RDMX = 0x05,
  ID_SDMX = 0x06,
  ID_RDM =  0x07,
  ID_RCMODE = 0x08,
  ID_COS = 0x09,
  ID_SNOREQ = 0x0A,
  ID_SNOREP = 0x0A
} usbpro_packet_type;

/*
 * Connect to the widget
 * @returns true if the connect succeeded, false otherwise
 */
bool UsbProWidget::Connect(const string &path) {
  if (m_enabled)
    return false;

  if (m_socket) {
    m_socket->Close();
    delete m_socket;
  }

  struct termios newtio;
  int fd = open(path.data(), O_RDWR | O_NONBLOCK | O_NOCTTY);

  if (fd == -1) {
    OLA_WARN << "Failed to open " << path << " " << strerror(errno);
    return false;
  }

  bzero(&newtio, sizeof(newtio)); // clear struct for new port settings
  tcsetattr(fd, TCSANOW, &newtio);
  m_socket = new ola::network::DeviceSocket(fd);
  m_socket->SetOnData(NewClosure(this, &UsbProWidget::SocketReady));

  // fire off a get request
  if (!GetParameters()) {
    OLA_WARN << "Failed to send a GetParameters message";
    delete m_socket;
    return false;
  }

  // put us into receiving mode
  if (!SendChangeMode(RCMODE_CHANGE)) {
    OLA_WARN << "Failed to set mode";
    delete m_socket;
    return false;
  }

  m_enabled = true;
  return true;
}


/*
 * Disconnect from the widget
 * @returns true if we disconnected, false otherwise
 */
bool UsbProWidget::Disconnect() {
  if (!m_enabled)
    return false;

  if (m_socket) {
    m_socket->Close();
    // we don't delete the socket here because ownership is transferred to the
    // SelectServer
    m_socket = NULL;
  }
  m_enabled = false;
  return true;
}


/*
 * Send a dmx msg
 * @returns true if we sent ok, false otherwise
 */
bool UsbProWidget::SendDMX(const DmxBuffer &buffer) const {
  unsigned int length = std::min((unsigned int) DMX_BUFFER_LENGTH,
                                 buffer.Size());
  promsg msg;

  msg.som = SOM;
  msg.label = ID_SDMX;
  //start code to 0
  msg.pm_dmx.dmx[0] = K_START_CODE;
  buffer.Get(&msg.pm_dmx.dmx[1], length);
  set_msg_len(&msg, length + 1);
  return SendMessage(&msg);
}


/*
 * Send a rdm msg, rdm support is a bit sucky
 * @returns true if we sent ok, false otherwise
 */
bool UsbProWidget::SendRdm(const uint8_t *buf, unsigned int len) const {
  promsg msg;
  msg.som = SOM;
  msg.label = ID_RDM;
  set_msg_len(&msg, len);
  memcpy(&msg.pm_rdm.dmx, buf, len);
  return SendMessage(&msg);
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
  int l = std::min((unsigned int) USER_CONFIG_LENGTH, len);
  promsg msg;
  msg.som = SOM;
  msg.label = ID_PRMSET;
  set_msg_len(&msg, sizeof(pms_prmset) - USER_CONFIG_LENGTH + l);
  msg.pm_prmset.len = len & 0xFF;
  msg.pm_prmset.len_hi = (len & 0xFF) >> 8;
  brk = brk != K_MISSING_PARAM ? brk : m_break_time;
  mab = mab != K_MISSING_PARAM ? mab : m_mab_time;
  rate = rate != K_MISSING_PARAM ? rate : m_rate;

  if (brk == K_MISSING_PARAM ||
      mab == K_MISSING_PARAM ||
      rate == K_MISSING_PARAM) {
    OLA_WARN << "Missing default values for usb SetParam";
    return false;
  }
  msg.pm_prmset.brk = brk;
  msg.pm_prmset.mab = mab;
  msg.pm_prmset.rate = rate;
  memcpy(msg.pm_prmset.user, data, l);
  return SendMessage(&msg);
}


/*
 * Send a start reprogramming message.
 * @returns true if we sent ok, false otherwise
 */
bool UsbProWidget::SendReprogram() const {
  promsg msg;
  msg.som = SOM;
  msg.label = ID_PROGRAM_FIRMWARE;
  set_msg_len(&msg, 0);
  return SendMessage(&msg);
}


/*
 * Send a firmware packet
 * @returns true if we sent ok, false otherwise
 */
bool UsbProWidget::SendFirmwarePage(uint8_t *data,
                                    unsigned int data_length) const {
  unsigned int length = std::min(data_length,
                                 (unsigned int) FLASH_PAGE_LENGTH);
  promsg msg;
  msg.som = SOM;
  msg.label = ID_FLASH_PAGE;
  set_msg_len(&msg, length);
  memcpy(msg.pm_flash_request.data, data, length);
  return SendMessage(&msg);
}


/*
 * Fetch the widget parameters, causes listener->Parameters() to be called
 * later.
 * @returns true if we sent ok, false otherwise
 */
bool UsbProWidget::GetParameters() const {
  int user_size = 0;
  promsg msg;
  msg.som = SOM;
  msg.label = ID_PRMREQ;
  set_msg_len(&msg, sizeof(pms_prmreq));
  msg.pm_prmreq.len = user_size & 0xFF;
  msg.pm_prmreq.len_hi = (user_size & 0xFF) >> 8;
  return SendMessage(&msg);
}


/*
 * Fetch the serial number, causes listener->SerialNumber() to be called
 * sometime later.
 * @returns true if we sent ok, false otherwise
 */
bool UsbProWidget::GetSerial() const {
  promsg msg;
  msg.som = SOM;
  msg.label = ID_SNOREQ;
  set_msg_len(&msg, 0);
  return SendMessage(&msg);
}


/*
 * Return the latest DmxData
 * @returns the DmxBuffer with the data
 */
const DmxBuffer &UsbProWidget::FetchDMX() const {
  return m_buffer;
}


/*
 * Force the widget back into receiving mode
 * @returns true if successfull, false otherwise
 */
bool UsbProWidget::ChangeToReceiveMode() {
  return SendChangeMode(RCMODE_CHANGE);
}


/*
 * Read data from the widget
 */
int UsbProWidget::SocketReady() {
  while (m_socket->DataRemaining() > 0) {
    ReceiveMessage();
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
bool UsbProWidget::SendChangeMode(int new_mode) {
  promsg msg;

  msg.som = SOM;
  msg.label = ID_RCMODE;
  set_msg_len(&msg, sizeof(pms_rcmode));
  msg.pm_rcmode.mode = new_mode;

  bool status = SendMessage(&msg);

  if (status && new_mode == RCMODE_CHANGE)
    m_buffer.Blackout();
  return status;
}


/*
 * Handle the flash page reply
 */
int UsbProWidget::handle_flash_page(pms_flash_reply *reply, int len) {
  bool status = false;
  if (len == FLASH_STATUS_LENGTH &&
      0 == memcmp(reply->status, REPLY_SUCCESS, len))
    status = true;

  if (m_listener)
    m_listener->HandleFirmwareReply(status);
  return 0;
}

/*
 * Handle the dmx frame
 * Don't do anything as we expect change of state messages instead
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
  int chn_st = cos->start * 8;

  // should be checking length here
  int offset = 0;
  for (int i = 0; i < 40; i++) {
    if (chn_st + i > DMX_BUFFER_LENGTH - 1 || offset + 6 >= len)
      break;

    if (cos->changed[i/8] & (1 << (i % 8)) ) {
      m_buffer.SetChannel(chn_st + i, cos->data[offset]);
      offset++;
    }
  }

  if (m_listener)
    m_listener->HandleWidgetDmx();

  return 0;
}


/*
 * Handle the param reply
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
 * @return true if successfull, false otherwise
 */
bool UsbProWidget::SendMessage(promsg *msg) const {
  int len = (msg->len_hi << 8) + msg->len + K_HEADER_SIZE;
  ssize_t bytes_sent = m_socket->Send((uint8_t*) msg, len);
  if (bytes_sent != len)
    return false;
  uint8_t eom = EOM;
  bytes_sent = m_socket->Send(&eom, sizeof(EOM));
  if (bytes_sent != sizeof(EOM))
    return false;
  return true;
}


/*
 * Read the data and handle the messages.
 */
int UsbProWidget::ReceiveMessage() {
  uint8_t byte = 0x00;
  uint8_t label;
  int plen, bytes_read;
  unsigned int cnt;
  pmu buf;

  while(byte != 0x7e) {
    m_socket->Receive((uint8_t*) &byte, 1, cnt);
    if (cnt == 1)
      continue;
    return -1;
  }

  // try to read the label
  m_socket->Receive((uint8_t*) &label, 1, cnt);

  if (cnt != 1) {
    OLA_WARN << "Could not read label, expected 1, got " << cnt;
    return 1;
  }

  m_socket->Receive((uint8_t*) &byte, 1, cnt);
  if (cnt != 1) {
    OLA_WARN << "Could not read len hi, expected 1, got " << cnt;
    return 1;
  }
  plen = byte;

  m_socket->Receive((uint8_t*) &byte, 1, cnt);
  if (cnt != 1) {
    OLA_WARN << "Could not read len lo, expected 1, got " << cnt;
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
    OLA_WARN << "Read to much, expected 1, got " << cnt;
    return 1;
  }

  if (byte == 0xe7) {
    switch(label) {
      case ID_FLASH_PAGE:
        handle_flash_page(&buf.pmu_flash_reply, plen);
        break;
      case ID_RDMX:
        handle_dmx(&buf.pmu_rdmx, plen);
        break;
      case ID_PRMREP:
        handle_prmrep(&buf.pmu_prmrep, plen);
        break;
      case ID_COS:
        handle_cos(&buf.pmu_cos, plen);
        break;
      case ID_SNOREP:
        handle_snorep(&buf.pmu_snorep, plen);
        break;
      default:
        OLA_WARN << "Unknown message type " << label;
    }
  }
  return 0;
}

} // usbpro
} //ola
