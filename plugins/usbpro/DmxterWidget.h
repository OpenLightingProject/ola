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
 * DmxterWidget.h
 * The Goddard Design Dmxter RDM and miniDmxter
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_DMXTERWIDGET_H_
#define PLUGINS_USBPRO_DMXTERWIDGET_H_

#include "ola/network/SelectServerInterface.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/QueueingRDMController.h"
#include "plugins/usbpro/UsbWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * A DMXter Widget implementation. We separate the Widget from the
 * implementation so we can leverage the QueueingRDMController.
 */
class DmxterWidgetImpl: public ola::rdm::RDMControllerInterface {
  public:
    DmxterWidgetImpl(ola::network::SelectServerInterface *ss,
                     UsbWidgetInterface *widget,
                     uint16_t esta_id,
                     uint32_t serial);
    ~DmxterWidgetImpl();

    void SetUIDListCallback(
        ola::Callback1<void, const ola::rdm::UIDSet&> *callback);

    void HandleMessage(uint8_t label,
                       const uint8_t *data,
                       unsigned int length);

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete);

    void RunRDMDiscovery();
    void SendUIDUpdate();
    void SendTodRequest();

  private:
    ola::rdm::UID m_uid;
    UsbWidgetInterface *m_widget;
    ola::network::SelectServerInterface *m_ss;
    ola::rdm::UIDSet m_uids;
    ola::Callback1<void, const ola::rdm::UIDSet&> *m_uid_set_callback;
    ola::rdm::RDMCallback *m_rdm_request_callback;
    uint8_t m_transaction_number;

    void HandleTodResponse(const uint8_t *data, unsigned int length);
    void HandleRDMResponse(const uint8_t *data, unsigned int length);
    void HandleBroadcastRDMResponse(const uint8_t *data, unsigned int length);
    void HandleShutdown(const uint8_t *data, unsigned int length);

    static const uint8_t RDM_REQUEST_LABEL;
    static const uint8_t RDM_BCAST_REQUEST_LABEL;
    static const uint8_t TOD_LABEL;
    static const uint8_t DISCOVERY_BRANCH_LABEL;
    static const uint8_t SHUTDOWN_LABAEL;

    typedef enum {
      RC_CHECKSUM_ERROR = 1,
      RC_FRAMING_ERROR = 2,
      RC_FRAMING_ERROR2 = 3,
      RC_BAD_STARTCODE = 4,
      RC_BAD_SUB_STARTCODE = 5,
      RC_WRONG_PDL = 6,
      RC_BAD_PDL = 7,
      RC_PACKET_TOO_SHORT = 8,
      RC_PACKET_TOO_LONG = 9,
      RC_PHYSICAL_LENGTH_MISMATCH = 10,
      RC_PDL_LENGTH_MISMATCH = 11,
      RC_TRANSACTION_MISMATCH = 12,
      RC_BAD_RESPONSE_TYPE = 13,
      RC_GOOD_RESPONSE = 14,
      RC_ACK_TIMER = 15,
      RC_ACK_OVERFLOW = 16,
      RC_TIMED_OUT = 17,
      RC_IDLE_LEVEL = 18,
      RC_GOOD_LEVEL = 19,
      RC_BAD_LEVEL = 20,
      RC_BROADCAST = 21,
      RC_VENDORCAST = 22,
      RC_NACK = 23,
      RC_NACK_UNKNOWN_PID = 24,
      RC_NACK_FORMAT_ERROR = 25,
      RC_NACK_HARDWARE_FAULT = 26,
      RC_NACK_PROXY_REJECT = 27,
      RC_NACK_WRITE_PROECT = 28,
      RC_NACK_COMMAND_CLASS = 29,
      RC_NACK_DATA_RANGE = 30,
      RC_NACK_BUFFER_FULL = 31,
      RC_NACK_PACKET_SIZE = 32,
      RC_NACK_SUB_DEVICE_RANGE = 33,
      RC_NACK_PROXY_QUEUE_BUFFER_FULL = 34,
      RC_DEST_UID_MISMATCH = 40,
      RC_SRC_UID_MISMATCH = 41,
      RC_SUBDEVICE_MISMATCH = 42,
      RC_COMMAND_CLASS_MISMATCH = 43,
      RC_PARAM_ID_MISMATCH = 44,
    } response_code;
};


/*
 * A DMXter Widget. This mostly just wraps the implementation.
 */
class DmxterWidget: public ola::rdm::RDMControllerInterface {
  public:
    DmxterWidget(ola::network::SelectServerInterface *ss,
                 UsbWidgetInterface *widget,
                 uint16_t esta_id,
                 uint32_t serial,
                 unsigned int queue_size = 20);
    ~DmxterWidget() {}

    void SetUIDListCallback(
        ola::Callback1<void, const ola::rdm::UIDSet&> *callback) {
      m_impl.SetUIDListCallback(callback);
    }

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete) {
      m_controller.SendRDMRequest(request, on_complete);
    }

    void RunRDMDiscovery() {
      return m_impl.RunRDMDiscovery();
    }

    void SendUIDUpdate() {
      return m_impl.SendUIDUpdate();
    }

    void SendTodRequest() {
      return m_impl.SendTodRequest();
    }

  private:
    DmxterWidgetImpl m_impl;
    ola::rdm::QueueingRDMController m_controller;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_DMXTERWIDGET_H_
