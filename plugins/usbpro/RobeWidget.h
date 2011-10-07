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
 * RobeWidget.h
 * Read and Write to a USB Widget.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_ROBEWIDGET_H_
#define PLUGINS_USBPRO_ROBEWIDGET_H_

#include <stdint.h>
#include <ola/Callback.h>
#include <ola/DmxBuffer.h>
#include "ola/network/Socket.h"
#include "plugins/usbpro/SerialWidgetInterface.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * A Robe USB Widget.
 */
class RobeWidget: public SerialWidgetInterface {
  public:
    explicit RobeWidget(ola::network::ConnectedDescriptor *descriptor);
    ~RobeWidget();

    typedef ola::Callback3<void, uint8_t, const uint8_t*, unsigned int>
      MessageHandler;

    ola::network::ConnectedDescriptor *GetDescriptor() const {
      return m_descriptor;
    }

    void SetMessageHandler(MessageHandler *callback);
    void SetOnRemove(ola::SingleUseCallback0<void> *on_close);

    void DescriptorReady();

    bool SendDMX(const DmxBuffer &buffer);

    bool SendMessage(uint8_t label,
                     const uint8_t *data,
                     unsigned int length) const;

    static const uint8_t CHANNEL_A_OUT = 0x06;
    static const uint8_t INFO_REQUEST = 0x14;
    static const uint8_t INFO_RESPONSE = 0x15;
    static const int DMX_FRAME_DATA_SIZE;

  private:
    typedef enum {
      PRE_SOM,
      RECV_PACKET_TYPE,
      RECV_SIZE_LO,
      RECV_SIZE_HI,
      RECV_HEADER_CRC,
      RECV_BODY,
      RECV_CRC,
    } receive_state;

    enum {MAX_DATA_SIZE = 522};

    typedef struct {
      uint8_t som;
      uint8_t packet_type;
      uint8_t len;
      uint8_t len_hi;
      uint8_t header_crc;
    } message_header;

    ola::Callback3<void, uint8_t, const uint8_t*, unsigned int> *m_callback;
    ola::network::ConnectedDescriptor *m_descriptor;
    receive_state m_state;
    unsigned int m_bytes_received, m_data_size;
    uint8_t m_crc;
    message_header m_header;
    uint8_t m_recv_buffer[MAX_DATA_SIZE];

    void ReceiveMessage();

    static const uint8_t SOM = 0xa5;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_ROBEWIDGET_H_
