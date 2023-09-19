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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * DmxterWidget.h
 * The Goddard Design Dmxter RDM and miniDmxter
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_DMXTERWIDGET_H_
#define PLUGINS_USBPRO_DMXTERWIDGET_H_

#include <memory>
#include "ola/io/SelectServerInterface.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/QueueingRDMController.h"
#include "plugins/usbpro/BaseUsbProWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * A DMXter Widget implementation. We separate the Widget from the
 * implementation so we can leverage the QueueingRDMController.
 */
class DmxterWidgetImpl: public BaseUsbProWidget,
                        public ola::rdm::DiscoverableRDMControllerInterface {
 public:
    DmxterWidgetImpl(ola::io::ConnectedDescriptor *descriptor,
                     uint16_t esta_id,
                     uint32_t serial);
    ~DmxterWidgetImpl();

    void Stop();

    void SendRDMRequest(ola::rdm::RDMRequest *request_ptr,
                        ola::rdm::RDMCallback *on_complete);

    void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
    void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);

 private:
    ola::rdm::UID m_uid;
    ola::rdm::UIDSet m_uids;
    ola::rdm::RDMDiscoveryCallback *m_discovery_callback;
    std::auto_ptr<const ola::rdm::RDMRequest> m_pending_request;
    ola::rdm::RDMCallback *m_rdm_request_callback;
    uint8_t m_transaction_number;

    void HandleMessage(uint8_t label,
                       const uint8_t *data,
                       unsigned int length);
    void HandleTodResponse(const uint8_t *data, unsigned int length);
    void HandleRDMResponse(const uint8_t *data,
                           unsigned int length);
    void HandleBroadcastRDMResponse(const uint8_t *data, unsigned int length);
    void HandleShutdown(const uint8_t *data, unsigned int length);

    static const uint8_t RDM_REQUEST_LABEL;
    static const uint8_t RDM_BCAST_REQUEST_LABEL;
    static const uint8_t TOD_LABEL;
    static const uint8_t DISCOVERY_BRANCH_LABEL;
    static const uint8_t FULL_DISCOVERY_LABEL;
    static const uint8_t INCREMENTAL_DISCOVERY_LABEL;
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
      RC_NACK_WRITE_PROTECT = 28,
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
      RC_DATA_RECEIVED_NO_BREAK = 46,
    } response_code;
};


/*
 * A DMXter Widget. This mostly just wraps the implementation.
 */
class DmxterWidget: public SerialWidgetInterface,
                    public ola::rdm::DiscoverableRDMControllerInterface {
 public:
    DmxterWidget(ola::io::ConnectedDescriptor *descriptor,
                 uint16_t esta_id,
                 uint32_t serial,
                 unsigned int queue_size = 20);
    ~DmxterWidget();

    void Stop() { m_impl->Stop(); }

    void SendRDMRequest(ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete) {
      m_controller->SendRDMRequest(request, on_complete);
    }

    void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      m_controller->RunFullDiscovery(callback);
    }

    void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      m_controller->RunIncrementalDiscovery(callback);
    }

    ola::io::ConnectedDescriptor *GetDescriptor() const {
      return m_impl->GetDescriptor();
    }

 private:
    // we need to control the order of construction & destruction here so these
    // are pointers.
    DmxterWidgetImpl *m_impl;
    ola::rdm::DiscoverableQueueingRDMController *m_controller;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_DMXTERWIDGET_H_
