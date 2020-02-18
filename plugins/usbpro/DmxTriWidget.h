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
 * DmxTriWidget.h
 * The Jese DMX-TRI/RDM-TRI widget.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_DMXTRIWIDGET_H_
#define PLUGINS_USBPRO_DMXTRIWIDGET_H_

#include <map>
#include <string>
#include <queue>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/rdm/QueueingRDMController.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/UIDSet.h"
#include "ola/thread/SchedulerInterface.h"
#include "plugins/usbpro/BaseUsbProWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

/*
 * A DMX TRI Widget implementation. We separate the Widget from the
 * implementation so we can leverage the QueueingRDMController.
 */
class DmxTriWidgetImpl: public BaseUsbProWidget,
                        public ola::rdm::DiscoverableRDMControllerInterface {
 public:
    DmxTriWidgetImpl(ola::thread::SchedulerInterface *executor,
                     ola::io::ConnectedDescriptor *descriptor,
                     bool use_raw_rdm);
    ~DmxTriWidgetImpl();

    void UseRawRDM(bool use_raw_rdm) { m_use_raw_rdm = use_raw_rdm; }

    void Stop();

    bool SendDMX(const DmxBuffer &buffer);
    void SendRDMRequest(ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete);
    void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
    void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);

 private:
    typedef enum {
      SINGLE_TX_COMMAND_ID = 0x21,
      DISCOVER_AUTO_COMMAND_ID = 0x33,
      DISCOVER_STATUS_COMMAND_ID = 0x34,
      REMOTE_UID_COMMAND_ID = 0x35,
      RAW_RDM_COMMAND_ID = 0x37,
      REMOTE_GET_COMMAND_ID = 0x38,
      REMOTE_SET_COMMAND_ID = 0x39,
      QUEUED_GET_COMMAND_ID = 0x3a,
      SET_FILTER_COMMAND_ID = 0x3d,

      RESERVED_COMMAND_ID = 0xff,
    } TriCommandId;

    typedef enum {
      NO_DISCOVERY_ACTION,
      DISCOVER_AUTO_REQUIRED,
      DISCOVER_STATUS_REQUIRED,
      FETCH_UID_REQUIRED,
    } TriDiscoveryState;

    typedef std::map<ola::rdm::UID, uint8_t> UIDToIndexMap;

    ola::thread::SchedulerInterface *m_scheduler;
    UIDToIndexMap m_uid_index_map;
    uint8_t m_uid_count;
    uint16_t m_last_esta_id;
    bool m_use_raw_rdm;

    // State for sending DMX
    DmxBuffer m_outgoing_dmx;

    // State for handling RDM discovery
    ola::thread::timeout_id m_disc_stat_timeout_id;
    ola::rdm::RDMDiscoveryCallback *m_discovery_callback;
    TriDiscoveryState m_discovery_state;

    // State for sending RDM Gets/Sets
    // This holds pointers to the RDMRequest and Callback that is queued or in
    // flight.
    ola::rdm::RDMCallback *m_rdm_request_callback;
    std::auto_ptr<ola::rdm::RDMRequest> m_pending_rdm_request;
    uint8_t m_transaction_number;
    // The command id that we expect to see in the response.
    uint8_t m_last_command, m_expected_command;

    void SendDMXBuffer();
    void SendQueuedRDMCommand();
    void RunDiscoveryCallback(ola::rdm::RDMDiscoveryCallback *callback);
    bool CheckDiscoveryStatus();
    void HandleMessage(uint8_t label,
                       const uint8_t *data,
                       unsigned int length);
    void SendDiscoveryAuto();
    void SendDiscoveryStat();
    void FetchNextUID();
    void SendRawRDMRequest();
    void DispatchRequest();
    void DispatchQueuedGet();
    void StopDiscovery();

    void HandleSingleTXResponse(uint8_t return_code);
    void HandleDiscoveryAutoResponse(uint8_t return_code,
                                     const uint8_t *data,
                                     unsigned int length);
    void HandleDiscoverStatResponse(uint8_t return_code,
                                    const uint8_t *data,
                                    unsigned int length);
    void HandleRemoteUIDResponse(uint8_t return_code,
                                 const uint8_t *data,
                                 unsigned int length);
    void HandleRawRDMResponse(uint8_t return_code,
                              const uint8_t *data,
                              unsigned int length);
    void HandleRemoteRDMResponse(uint8_t return_code,
                                 const uint8_t *data,
                                 unsigned int length);
    void HandleQueuedGetResponse(uint8_t return_code,
                                 const uint8_t *data,
                                 unsigned int length);
    void HandleGenericRDMResponse(uint8_t return_code,
                                  uint16_t pid,
                                  const uint8_t *data,
                                  unsigned int length);
    void HandleSetFilterResponse(uint8_t return_code,
                                 const uint8_t *data,
                                 unsigned int length);
    bool PendingTransaction() const;
    void MaybeSendNextRequest();
    void HandleRDMError(ola::rdm::RDMStatusCode error_code);
    bool SendCommandToTRI(uint8_t label, const uint8_t *data,
                          unsigned int length);
    bool TriToOlaReturnCode(uint8_t return_code,
                            ola::rdm::RDMStatusCode *code);
    bool ReturnCodeToNackReason(uint8_t return_code,
                                ola::rdm::rdm_nack_reason *reason);

    typedef enum {
      EC_NO_ERROR = 0,
      EC_CONSTRAINT = 1,
      EC_UNKNOWN_COMMAND = 2,
      EC_INVALID_OPTION = 3,
      EC_FRAME_FORMAT = 4,
      EC_DATA_TOO_LONG = 5,
      EC_DATA_MISSING = 6,
      EC_SYSTEM_MODE = 7,
      EC_SYSTEM_BUSY = 8,
      EC_DATA_CHECKSUM = 0x0a,
      EC_INCOMPATIBLE = 0x0b,
      EC_RESPONSE_TIME = 0x10,
      EC_RESPONSE_WAIT = 0x11,
      EC_RESPONSE_MORE = 0x12,
      EC_RESPONSE_TRANSACTION = 0x13,
      EC_RESPONSE_SUB_DEVICE = 0x14,
      EC_RESPONSE_FORMAT = 0x15,
      EC_RESPONSE_CHECKSUM = 0x16,
      EC_RESPONSE_NONE = 0x18,
      EC_RESPONSE_IDENTITY = 0x1a,
      EC_RESPONSE_MUTE = 0x1b,
      EC_RESPONSE_DISCOVERY = 0x1c,
      EC_RESPONSE_UNEXPECTED = 0x1d,
      EC_UNKNOWN_PID = 0x20,
      EC_FORMAT_ERROR = 0x21,
      EC_HARDWARE_FAULT = 0x22,
      EC_PROXY_REJECT = 0x23,
      EC_WRITE_PROTECT = 0x24,
      EC_UNSUPPORTED_COMMAND_CLASS = 0x25,
      EC_OUT_OF_RANGE = 0x26,
      EC_BUFFER_FULL = 0x27,
      EC_FRAME_OVERFLOW = 0x28,
      EC_SUBDEVICE_UNKNOWN = 0x29,
      EC_PROXY_BUFFER_FULL = 0x2a,  // this isn't included in the docs
      EC_ACTION_NOT_SUPPORTED = 0x2b,  // this is a guess
      EC_ENDPOINT_NUMBER_INVALID = 0x2c,  // this is a guess
      EC_INVALID_ENDPOINT_MODE = 0x2d,  // this is a guess
      EC_UNKNOWN_UID = 0x2e,  // this is a guess
      EC_UNKNOWN_SCOPE = 0x2f,  // this is a guess
      EC_INVALID_STATIC_CONFIG_TYPE = 0x30,  // this is a guess
      EC_INVALID_IPV4_ADDRESS = 0x31,  // this is a guess
      EC_INVALID_IPV6_ADDRESS = 0x32,  // this is a guess
      EC_INVALID_PORT = 0x33  // this is a guess
    } dmx_tri_error_codes;
    // TODO(Peter): try and test the guessed values

    static const unsigned int DATA_OFFSET = 2;  // first two bytes are CI & RC
    static const uint8_t EXTENDED_COMMAND_LABEL = 88;  // 'X'

    // The ms delay between checking on the RDM discovery process
    static const unsigned int RDM_STATUS_INTERVAL_MS = 100;
};


/*
 * A DMX TRI Widget
 */
class DmxTriWidget: public SerialWidgetInterface,
                    public ola::rdm::DiscoverableRDMControllerInterface {
 public:
    DmxTriWidget(ola::thread::SchedulerInterface *ss,
                 ola::io::ConnectedDescriptor *descriptor,
                 unsigned int queue_size = 20,
                 bool use_raw_rdm = false);
    ~DmxTriWidget();
    void UseRawRDM(bool use_raw_rdm) { m_impl->UseRawRDM(use_raw_rdm); }

    void Stop() { m_impl->Stop(); }

    bool SendDMX(const DmxBuffer &buffer) const {
      return m_impl->SendDMX(buffer);
    }

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
    DmxTriWidgetImpl *m_impl;
    ola::rdm::DiscoverableQueueingRDMController *m_controller;

    void ResumeRDMCommands() {
      m_controller->Resume();
    }
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_DMXTRIWIDGET_H_
