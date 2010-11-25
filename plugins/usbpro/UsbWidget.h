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
 * UsbWidget.h
 * Read and Write to a USB Widget.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBWIDGET_H_
#define PLUGINS_USBPRO_USBWIDGET_H_

#include <stdint.h>
#include <ola/Callback.h>
#include <string>
#include "ola/network/Socket.h"

namespace ola {
namespace plugin {
namespace usbpro {



/*
 * The basic representation of a USB widget. This knows how to send and recieve
 * usb pro messages.
 */
class UsbWidget {
  public:
    explicit UsbWidget(ola::network::ConnectedSocket *socket);
    ~UsbWidget();
    void SetMessageHandler(
      ola::Callback3<void, uint8_t, unsigned int, const uint8_t*> *callback);
    void SetOnRemove(ola::SingleUseCallback0<void> *on_close);

    void SocketReady();

    bool SendMessage(uint8_t label, unsigned int length,
                     const uint8_t *data) const;

    static ola::network::ConnectedSocket *OpenDevice(const string &path);

    // we put these here so we don't have to duplicate them.
    static const uint8_t DMX_LABEL = 6;
    static const uint8_t SERIAL_LABEL = 10;
    static const uint8_t MANUFACTURER_LABEL = 77;
    static const uint8_t DEVICE_LABEL = 78;

  private:
    typedef enum {
      PRE_SOM,
      RECV_LABEL,
      RECV_SIZE_LO,
      RECV_SIZE_HI,
      RECV_BODY,
      RECV_EOM,
    } receive_state;

    enum {MAX_DATA_SIZE = 600};

    typedef struct {
      uint8_t som;
      uint8_t label;
      uint8_t len;
      uint8_t len_hi;
    } message_header;

    ola::Callback3<void, uint8_t, unsigned int, const uint8_t*> *m_callback;
    ola::network::ConnectedSocket *m_socket;
    receive_state m_state;
    unsigned int m_bytes_received;
    message_header m_header;
    uint8_t m_recv_buffer[MAX_DATA_SIZE];

    void ReceiveMessage();

    static const uint8_t EOM = 0xe7;
    static const uint8_t SOM = 0x7e;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_USBWIDGET_H_
