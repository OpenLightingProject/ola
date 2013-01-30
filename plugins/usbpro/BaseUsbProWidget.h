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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * BaseUsbProWidget.h
 * Read and Write to a USB Serial Widget that uses the Enttec Usb Pro frame
 * layout.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_BASEUSBPROWIDGET_H_
#define PLUGINS_USBPRO_BASEUSBPROWIDGET_H_

#include <stdint.h>
#include <string>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/io/Descriptor.h"
#include "plugins/usbpro/SerialWidgetInterface.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * A widget that implements the Usb Pro frame format.
 */
class BaseUsbProWidget: public SerialWidgetInterface {
  public:
    explicit BaseUsbProWidget(ola::io::ConnectedDescriptor *descriptor);
    virtual ~BaseUsbProWidget();

    ola::io::ConnectedDescriptor *GetDescriptor() const {
      return m_descriptor;
    }
    void DescriptorReady();

    // we locate the SendDMX in the base class since so many widgets share it.
    virtual bool SendDMX(const DmxBuffer &buffer);

    bool SendMessage(uint8_t label,
                     const uint8_t *data,
                     unsigned int length) const;

    static ola::io::ConnectedDescriptor *OpenDevice(const string &path);

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

    ola::io::ConnectedDescriptor *m_descriptor;
    receive_state m_state;
    unsigned int m_bytes_received;
    message_header m_header;
    uint8_t m_recv_buffer[MAX_DATA_SIZE];

    void ReceiveMessage();
    virtual void HandleMessage(uint8_t label,
                               const uint8_t *data,
                               unsigned int length) = 0;

    static const uint8_t EOM = 0xe7;
    static const uint8_t SOM = 0x7e;
    static const unsigned int HEADER_SIZE;
};


/**
 * A Usb Pro Widget that can execute a callback when it receives messages.
 * This is used for discovery.
 */
class DispatchingUsbProWidget: public BaseUsbProWidget {
  public:
    typedef ola::Callback3<void,
                           uint8_t,
                           const uint8_t*,
                           unsigned int> MessageCallback;
    DispatchingUsbProWidget(ola::io::ConnectedDescriptor *descriptor,
                            MessageCallback *callback)
        : BaseUsbProWidget(descriptor),
          m_callback(callback) {
    }

    ~DispatchingUsbProWidget() {
      Stop();
    }

    void Stop() {
      if (m_callback)
        delete m_callback;
    }

    void SetHandler(MessageCallback *callback) {
      if (m_callback)
        delete m_callback;
      m_callback = callback;
    }

  private:
    MessageCallback *m_callback;

    void HandleMessage(uint8_t label,
                       const uint8_t *data,
                       unsigned int length) {
      m_callback->Run(label, data, length);
    }
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_BASEUSBPROWIDGET_H_
