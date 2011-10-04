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
 * ArduinoWidget.h
 * The Arduino RGB Mixer widget.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_ARDUINOWIDGET_H_
#define PLUGINS_USBPRO_ARDUINOWIDGET_H_

#include "ola/DmxBuffer.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/QueueingRDMController.h"
#include "plugins/usbpro/BaseUsbProWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * A Arduino Widget implementation. We separate the Widget from the
 * implementation so we can leverage the QueueingRDMController.
 */
class ArduinoWidgetImpl: public BaseUsbProWidget,
                         public ola::rdm::DiscoverableRDMControllerInterface {
  public:
    ArduinoWidgetImpl(ola::network::ConnectedDescriptor *descriptor,
                      uint16_t esta_id,
                      uint32_t serial);
    ~ArduinoWidgetImpl();

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete);

    bool RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      return GetUidSet(callback);
    }

    bool RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      return GetUidSet(callback);
    }

  private:
    uint8_t m_transaction_id;
    ola::rdm::UID m_uid;
    const ola::rdm::RDMRequest *m_pending_request;
    ola::rdm::RDMCallback *m_rdm_request_callback;

    void HandleMessage(uint8_t label,
                       const uint8_t *data,
                       unsigned int length);
    void HandleRDMResponse(const uint8_t *data, unsigned int length);
    bool GetUidSet(ola::rdm::RDMDiscoveryCallback *callback);

    static const uint8_t RDM_REQUEST_LABEL;

    // the first byte is the response code
    static const uint8_t RESPONSE_OK;
    static const uint8_t RESPONSE_WAS_BROADCAST;
    static const uint8_t RESPONSE_FAILED;
    static const uint8_t RESPONSE_FAILED_CHECKSUM;
    static const uint8_t RESONSE_INVALID_DESTINATION;
    static const uint8_t RESPONSE_INVALID_COMMAND;
};


/*
 * A Arduino Widget. This mostly just wraps the implementation.
 */
class ArduinoWidget: public SerialWidgetInterface,
                     public ola::rdm::DiscoverableRDMControllerInterface {
  public:
    ArduinoWidget(ola::network::ConnectedDescriptor *descriptor,
                  uint16_t esta_id,
                  uint32_t serial,
                  unsigned int queue_size = 20);
    ~ArduinoWidget();

    bool SendDMX(const DmxBuffer &buffer) {
      return m_impl->SendDMX(buffer);
    }

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete) {
      m_controller->SendRDMRequest(request, on_complete);
    }

    bool RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      return m_impl->RunFullDiscovery(callback);
    }

    bool RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      return m_impl->RunIncrementalDiscovery(callback);
    }

    ola::network::ConnectedDescriptor *GetDescriptor() const {
      return m_impl->GetDescriptor();
    }

    void SetOnRemove(ola::SingleUseCallback0<void> *on_close) {
      m_impl->SetOnRemove(on_close);
    }

    void CloseDescriptor() { m_impl->CloseDescriptor(); }

  private:
    // we need to control the order of construction & destruction here so these
    // are pointers.
    ArduinoWidgetImpl *m_impl;
    ola::rdm::DiscoverableQueueingRDMController *m_controller;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_ARDUINOWIDGET_H_
