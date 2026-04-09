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
 * EnttecUsbProWidget.cpp
 * The Enttec USB Pro Widget
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ola/Callback.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/io/ByteString.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/stl/STLUtils.h"
#include "ola/strings/Format.h"
#include "plugins/usbpro/BaseUsbProWidget.h"
#include "plugins/usbpro/EnttecUsbProWidget.h"
#include "plugins/usbpro/EnttecUsbProWidgetImpl.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::io::ByteString;
using ola::rdm::RDMCommand;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMReply;
using ola::rdm::RunRDMCallback;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::strings::IntToString;
using ola::strings::ToHex;
using std::auto_ptr;
using std::string;
using std::vector;


const uint16_t EnttecUsbProWidget::ENTTEC_ESTA_ID = 0x454E;

OperationLabels OperationLabels::Port1Operations() {
  OperationLabels ops = {
    GET_PARAMS_1,
    SET_PARAMS_1,
    RECEIVED_DMX_LABEL_1,
    SEND_DMX_1,
    SEND_RDM_1,
    CHANGE_TO_RX_1,
    COS_DMX_1,
    RDM_TIMEOUT_1,
    RDM_DISCOVERY_1,
  };
  return ops;
}

OperationLabels OperationLabels::Port2Operations() {
  // These labels are specific to OLA and are tied to the OLA API key.
  OperationLabels ops = {
    GET_PARAMS_2,
    SET_PARAMS_2,
    RECEIVED_DMX_LABEL_2,
    SEND_DMX_2,
    SEND_RDM_2,
    CHANGE_TO_RX_2,
    COS_DMX_2,
    RDM_TIMEOUT_2,
    RDM_DISCOVERY_2,
  };
  return ops;
}

EnttecPortImpl::EnttecPortImpl(const OperationLabels &ops, const UID &uid,
                               SendCallback *send_cb, bool no_rdm_dub_timeout)
    : m_send_cb(send_cb),
      m_ops(ops),
      m_active(true),
      m_watchdog(WATCHDOG_LIMIT,
                 NewCallback(this, &EnttecPortImpl::WatchdogFired)),
      m_discovery_agent(this),
      m_uid(uid),
      m_transaction_number(0),
      m_rdm_request_callback(NULL),
      m_pending_request(NULL),
      m_mute_callback(NULL),
      m_unmute_callback(NULL),
      m_branch_callback(NULL),
      m_discovery_response(NULL),
      m_discovery_response_size(0),
      m_no_rdm_dub_timeout(no_rdm_dub_timeout) {
}

void EnttecPortImpl::Stop() {
  m_active = false;

  m_dmx_callback.reset();

  // empty params struct
  usb_pro_parameters params;
  while (!m_outstanding_param_callbacks.empty()) {
    usb_pro_params_callback *callback = m_outstanding_param_callbacks.front();
    m_outstanding_param_callbacks.pop_front();
    callback->Run(false, params);
  }

  m_discovery_agent.Abort();
}


/**
 * Send a DMX message
 */
bool EnttecPortImpl::SendDMX(const DmxBuffer &buffer) {
  struct {
    uint8_t start_code;
    uint8_t dmx[DMX_UNIVERSE_SIZE];
  } widget_dmx;

  widget_dmx.start_code = DMX512_START_CODE;
  unsigned int length = DMX_UNIVERSE_SIZE;
  buffer.Get(widget_dmx.dmx, &length);
  return m_send_cb->Run(m_ops.send_dmx, reinterpret_cast<uint8_t*>(&widget_dmx),
                        length + 1);
}


/**
 * Set the callback to run when new DMX data arrives
 */
void EnttecPortImpl::SetDMXCallback(ola::Callback0<void> *callback) {
  m_dmx_callback.reset(callback);
}

/*
 * Put the device back into recv mode
 * @return true on success, false on failure
 */
bool EnttecPortImpl::ChangeToReceiveMode(bool change_only) {
  if (!m_active) {
    return false;
  }

  uint8_t mode = change_only;
  bool status = m_send_cb->Run(m_ops.change_to_rx_mode, &mode, sizeof(mode));
  if (status && change_only) {
    m_input_buffer.Blackout();
  }
  return status;
}


/**
 * Send a request for the widget's parameters.
 * TODO(simon): add timers to these
 */
void EnttecPortImpl::GetParameters(usb_pro_params_callback *callback) {
  m_outstanding_param_callbacks.push_back(callback);

  uint16_t user_size = 0;
  bool r = m_send_cb->Run(m_ops.get_params,
                          reinterpret_cast<uint8_t*>(&user_size),
                          sizeof(user_size));

  if (!r) {
    // failed
    m_outstanding_param_callbacks.pop_back();
    usb_pro_parameters params = {0, 0, 0, 0, 0};
    callback->Run(false, params);
  }
}


/**
 * Set the widget's parameters. Due to the lack of confirmation, this returns
 * immediately.
 */
bool EnttecPortImpl::SetParameters(uint8_t break_time,
                                   uint8_t mab_time,
                                   uint8_t rate) {
  PACK(
  struct widget_params_s {
    uint16_t length;
    uint8_t break_time;
    uint8_t mab_time;
    uint8_t rate;
  });

  widget_params_s widget_parameters = {
    0,
    break_time,
    mab_time,
    rate};

  bool ret = m_send_cb->Run(
      m_ops.set_params,
      reinterpret_cast<uint8_t*>(&widget_parameters),
      sizeof(widget_parameters));

  if (!ret) {
    OLA_WARN << "Failed to send a set params message";
  }
  return ret;
}


/**
 * Send an RDM Request.
 */
void EnttecPortImpl::SendRDMRequest(RDMRequest *request_ptr,
                                    ola::rdm::RDMCallback *on_complete) {
  auto_ptr<RDMRequest> request(request_ptr);

  if (m_rdm_request_callback) {
    OLA_WARN << "Previous request hasn't completed yet, dropping request";
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }

  request->SetSourceUID(m_uid);
  request->SetTransactionNumber(m_transaction_number++);
  request->SetPortId(PORT_ID);

  m_pending_request.reset(request.release());
  m_rdm_request_callback = on_complete;

  bool ok = PackAndSendRDMRequest(
      m_pending_request->IsDUB() ? m_ops.rdm_discovery : m_ops.send_rdm,
      m_pending_request.get());

  if (!ok) {
    m_pending_request.reset();
    m_rdm_request_callback = NULL;
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
  }
}


/**
 * Start full discovery for this widget.
 */
void EnttecPortImpl::RunFullDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  OLA_INFO << "Full discovery triggered";
  m_discovery_agent.StartFullDiscovery(
    ola::NewSingleCallback(this, &EnttecPortImpl::DiscoveryComplete, callback));
}


/**
 * Start incremental discovery for this widget
 */
void EnttecPortImpl::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  OLA_INFO << "Incremental discovery triggered";
  m_discovery_agent.StartIncrementalDiscovery(
    ola::NewSingleCallback(this, &EnttecPortImpl::DiscoveryComplete, callback));
}


/**
 * Mute a responder
 * @param target the UID to mute
 * @param mute_complete the callback to run once the mute request
 * completes.
 */
void EnttecPortImpl::MuteDevice(const ola::rdm::UID &target,
                                MuteDeviceCallback *mute_complete) {
  OLA_INFO << "Muting " << target << ", TN: "
           << static_cast<int>(m_transaction_number);
  auto_ptr<RDMRequest> mute_request(
      ola::rdm::NewMuteRequest(m_uid, target, m_transaction_number++));
  if (PackAndSendRDMRequest(m_ops.send_rdm, mute_request.get())) {
    m_mute_callback = mute_complete;
  } else {
    mute_complete->Run(false);
  }
}


/**
 * Unmute all responders
 * @param unmute_complete the callback to run once the unmute request
 * completes.
 */
void EnttecPortImpl::UnMuteAll(UnMuteDeviceCallback *unmute_complete) {
  OLA_INFO << "Un-muting all devices, TN: "
           << static_cast<int>(m_transaction_number);
  auto_ptr<RDMRequest> unmute_request(
      ola::rdm::NewUnMuteRequest(m_uid, ola::rdm::UID::AllDevices(),
                                 m_transaction_number++));
  if (PackAndSendRDMRequest(m_ops.send_rdm, unmute_request.get())) {
    m_unmute_callback = unmute_complete;
  } else {
    OLA_WARN << "Failed to send Unmute all request";
    unmute_complete->Run();
  }
}


/**
 * Send a Discovery Unique Branch
 */
void EnttecPortImpl::Branch(const ola::rdm::UID &lower,
                            const ola::rdm::UID &upper,
                            BranchCallback *callback) {
  auto_ptr<RDMRequest> branch_request(
      ola::rdm::NewDiscoveryUniqueBranchRequest(m_uid, lower, upper,
                                                m_transaction_number++));
  OLA_INFO << "Sending DUB packet: " << lower << " - " << upper;
  if (PackAndSendRDMRequest(m_ops.rdm_discovery, branch_request.get())) {
    m_branch_callback = callback;
  } else {
    callback->Run(NULL, 0);
  }
}


/**
 * Called to indicate the completion of an RDM request.
 * According to the spec:
 *  The timeout message will follow the RDM discovery reply message, whether or
 *   not the reply is partial or complete.
 *    This doesn't seem to be the case in reality for DUBs with newer firmware
       (>=4.15)
 *  The timeout message will follow the RDM reply message (GET or SET), only
 *   when the reply is incomplete or unrecognizable.
 *
 * Experiments suggest that sending another RDM message before this 'timeout'
 * is received results in Bad Things Happening.
 *
 * The length of this message should be 0.
 */
void EnttecPortImpl::HandleRDMTimeout(unsigned int length) {
  if (length) {
    OLA_WARN << "Strange RDM timeout message, length was " << length;
  }

  m_watchdog.Disable();

  // check what operation we were waiting on
  if (m_unmute_callback) {
    UnMuteDeviceCallback *callback = m_unmute_callback;
    m_unmute_callback = NULL;
    callback->Run();
  } else if (m_mute_callback) {
    MuteDeviceCallback *callback = m_mute_callback;
    m_mute_callback = NULL;
    OLA_INFO << "Unable to mute device";
    callback->Run(false);
  } else if (m_branch_callback) {
    BranchCallback *callback = m_branch_callback;
    m_branch_callback = NULL;
    callback->Run(m_discovery_response, m_discovery_response_size);
    if (m_discovery_response) {
      delete[] m_discovery_response;
      m_discovery_response = NULL;
      m_discovery_response_size = 0;
    }
  } else if (m_rdm_request_callback && m_pending_request.get()) {
    ola::rdm::RDMStatusCode status_code;
    if (m_pending_request->IsDUB()) {
        status_code = ola::rdm::RDM_TIMEOUT;
    } else {
      status_code = (
          m_pending_request->DestinationUID().IsBroadcast() ?
          ola::rdm::RDM_WAS_BROADCAST :
          ola::rdm::RDM_TIMEOUT);
    }

    ola::rdm::RDMCallback *callback = m_rdm_request_callback;
    m_rdm_request_callback = NULL;
    m_pending_request.reset();
    RunRDMCallback(callback, status_code);
  }
}


/*
 * Called when we get new parameters from the widget.
 */
void EnttecPortImpl::HandleParameters(const uint8_t *data,
                                      unsigned int length) {
  if (m_outstanding_param_callbacks.empty()) {
    return;
  }

  if (length < sizeof(usb_pro_parameters)) {
    return;
  }

  usb_pro_parameters params;
  memcpy(&params, data, sizeof(usb_pro_parameters));

  usb_pro_params_callback *callback = m_outstanding_param_callbacks.front();
  m_outstanding_param_callbacks.pop_front();

  callback->Run(true, params);
}


/**
 * Handle an incoming frame.
 * @param data the incoming data buffer
 * @param length the length of the data buffer.
 *
 * The first byte is a status code: 0: good, non-0: bad
 * The second byte is the start code
 * The remaining bytes are the actual data.
 */
void EnttecPortImpl::HandleIncomingDataMessage(const uint8_t *data,
                                               unsigned int length) {
  bool waiting_for_dub_response = (
      m_branch_callback != NULL || (
      (m_rdm_request_callback && m_pending_request->IsDUB())));

  // if we're not waiting for a DUB response, and this isn't an RDM frame, then
  // let the super class handle it.
  if (!waiting_for_dub_response && length >= 2 &&
      data[1] != ola::rdm::START_CODE) {
    HandleDMX(data, length);
    return;
  }

  // TODO(Simon): It's not clear what happens if we get an overrun on an RDM
  // response. Do we still get the timeout message or is this the only
  // response? I need to check with Nic.
  if (data[0]) {
    OLA_WARN << "Incoming frame corrupted";
    return;
  }

  m_watchdog.Disable();

  // skip over the status bit
  data++;
  length--;

  if (m_branch_callback) {
    // discovery responses are *always* followed by the timeout message and
    // it's important that we wait for this before sending the next command
    if (m_discovery_response) {
      OLA_WARN << "Multiple discovery responses received, ignoring all but "
                  "the first.";
      return;
    }
    uint8_t *response_data = new uint8_t[length];
    memcpy(response_data, data, length);
    m_discovery_response = response_data;
    m_discovery_response_size = length;
    if (m_no_rdm_dub_timeout) {
      // Newer firmware doesn't send an RDM timeout message on DUBs, so we fake
      // one to minimise changes to the code for older devices
      OLA_DEBUG << "Dummying HandleRDMTimeout(0) as device doesn't require it";
      HandleRDMTimeout(0);
    }
  } else if (m_mute_callback) {
    // we take any response as a mute acknowledgment here, which isn't great,
    // but it seems to work.
    MuteDeviceCallback *callback = m_mute_callback;
    m_mute_callback = NULL;
    OLA_INFO << "Probably muted device";
    callback->Run(true);
  } else if (m_rdm_request_callback) {
    ola::rdm::RDMCallback *callback = m_rdm_request_callback;
    m_rdm_request_callback = NULL;
    auto_ptr<const ola::rdm::RDMRequest> request(m_pending_request.release());
    auto_ptr<RDMReply> reply;
    if (waiting_for_dub_response) {
      reply.reset(RDMReply::DUBReply(rdm::RDMFrame(data, length)));
    } else {
      reply.reset(RDMReply::FromFrame(rdm::RDMFrame(data, length),
                                      request.get()));
    }
    callback->Run(reply.get());
  }
}


/*
 * Handle the dmx change of state frame
 */
void EnttecPortImpl::HandleDMXDiff(const uint8_t *data, unsigned int length) {
  typedef struct {
    uint8_t start;
    uint8_t changed[5];
    uint8_t data[40];
  } widget_data_changed;

  // The minimum packet length is 1 byte start_pos, 5 bytes bitmap
  // and one changed databyte. A length-6 packet would be valid (if
  // no bits in the changed bitmap was set), but nonsensical to
  // transmit. In any case, ignoring it would at least work.
  if ((length < 7) || (length > sizeof(widget_data_changed))) {
    OLA_WARN << "Change of state packet has an invalid length: " << length;
    return;
  }

  const widget_data_changed *widget_reply =
      reinterpret_cast<const widget_data_changed*>(data);

  unsigned int start_channel = widget_reply->start * 8;
  unsigned int offset = 0;

  // skip non-0 start codes, this code is pretty messed up because the USB Pro
  // doesn't seem to provide a guarantee on the ordering of packets. Packets
  // with non-0 start codes are almost certainly going to cause problems.
  if (start_channel == 0 && (widget_reply->changed[0] & 0x01) &&
      widget_reply->data[offset]) {
    return;
  }

  for (int i = 0; i < 40; i++) {
    if (start_channel + i > DMX_UNIVERSE_SIZE + 1 || offset + 6 >= length) {
      break;
    }

    if (widget_reply->changed[i/8] & (1 << (i % 8)) && start_channel + i != 0) {
      m_input_buffer.SetChannel(start_channel + i - 1,
                                widget_reply->data[offset]);
      offset++;
    }
  }

  if (m_dmx_callback.get()) {
    m_dmx_callback->Run();
  }
}


/*
 * Handle the dmx frame
 */
void EnttecPortImpl::HandleDMX(const uint8_t *data,
                               unsigned int length) {
  typedef struct {
    uint8_t status;
    uint8_t dmx[DMX_UNIVERSE_SIZE + 1];
  } widget_dmx;

  if (length < 2) {
    return;
  }

  const widget_dmx *widget_reply = reinterpret_cast<const widget_dmx*>(data);

  if (widget_reply->status) {
    OLA_WARN << "UsbPro got corrupted packet, status: "
             << static_cast<int>(widget_reply->status);
    return;
  }

  // only handle start code = 0
  if (length > 2 && widget_reply->dmx[0] == 0) {
    m_input_buffer.Set(widget_reply->dmx + 1, length - 2);
    if (m_dmx_callback.get()) {
      m_dmx_callback->Run();
    }
  }
  return;
}


/**
 * Called when the discovery process finally completes
 * @param callback the callback passed to StartFullDiscovery or
 * StartIncrementalDiscovery that we should execute.
 * @param status true if discovery worked, false otherwise
 * @param uids the UIDSet of UIDs that were found.
 */
void EnttecPortImpl::DiscoveryComplete(ola::rdm::RDMDiscoveryCallback *callback,
                                       bool,
                                       const UIDSet &uids) {
  OLA_DEBUG << "Enttec Pro discovery complete: " << uids;
  if (callback) {
    callback->Run(uids);
  }
}


/**
 * Send a RDM request to the widget
 */
bool EnttecPortImpl::PackAndSendRDMRequest(uint8_t label,
                                           const RDMRequest *request) {
  ByteString data;
  if (!RDMCommandSerializer::PackWithStartCode(*request, &data)) {
    return false;
  }

  bool ok = m_send_cb->Run(label, data.data(), data.size());
  if (ok) {
    m_watchdog.Enable();
  }
  return ok;
}

void EnttecPortImpl::ClockWatchdog() {
  m_watchdog.Clock();
}

void EnttecPortImpl::WatchdogFired() {
  if (m_branch_callback) {
    OLA_WARN << "Timeout waiting for DUB response";
    BranchCallback *callback = m_branch_callback;
    m_branch_callback = NULL;
    callback->Run(NULL, 0);
  } else if (m_mute_callback) {
    OLA_WARN << "Timeout waiting for mute response";
    MuteDeviceCallback *callback = m_mute_callback;
    m_mute_callback = NULL;
    callback->Run(false);
  } else if (m_unmute_callback) {
    OLA_WARN << "Timeout waiting for unmute response";
    UnMuteDeviceCallback *callback = m_unmute_callback;
    m_unmute_callback = NULL;
    callback->Run();
  } else if (m_rdm_request_callback) {
    OLA_WARN << "Timeout waiting for RDM response";
    ola::rdm::RDMCallback *callback = m_rdm_request_callback;
    m_rdm_request_callback = NULL;
    m_pending_request.reset();
    RunRDMCallback(callback, ola::rdm::RDM_FAILED_TO_SEND);
  }
}


// EnttecPort
// ----------------------------------------------------------------------------


/**
 * EnttecUsbProWidget Constructor
 */
EnttecPort::EnttecPort(EnttecPortImpl *impl, unsigned int queue_size,
                       bool enable_rdm)
    : m_impl(impl),
      m_enable_rdm(enable_rdm),
      m_controller(
          new ola::rdm::DiscoverableQueueingRDMController(m_impl, queue_size)) {
}

bool EnttecPort::SendDMX(const DmxBuffer &buffer) {
  return m_impl->SendDMX(buffer);
}

const DmxBuffer &EnttecPort::FetchDMX() const {
  return m_impl->FetchDMX();
}

void EnttecPort::SetDMXCallback(ola::Callback0<void> *callback) {
  m_impl->SetDMXCallback(callback);
}

bool EnttecPort::ChangeToReceiveMode(bool change_only) {
  return m_impl->ChangeToReceiveMode(change_only);
}

void EnttecPort::GetParameters(usb_pro_params_callback *callback) {
  m_impl->GetParameters(callback);
}

bool EnttecPort::SetParameters(uint8_t break_time, uint8_t mab_time,
                               uint8_t rate) {
  return m_impl->SetParameters(break_time, mab_time, rate);
}

void EnttecPort::SendRDMRequest(ola::rdm::RDMRequest *request,
                                ola::rdm::RDMCallback *on_complete) {
  if (m_enable_rdm) {
    m_controller->SendRDMRequest(request, on_complete);
  } else {
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
    delete request;
  }
}

void EnttecPort::RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
  if (m_enable_rdm) {
    m_controller->RunFullDiscovery(callback);
  } else {
    UIDSet uids;
    callback->Run(uids);
  }
}

void EnttecPort::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  if (m_enable_rdm) {
    m_controller->RunIncrementalDiscovery(callback);
  } else {
    UIDSet uids;
    callback->Run(uids);
  }
}


// EnttecUsbProWidgetImpl
// ----------------------------------------------------------------------------

/*
 * An Enttec DMX USB PRO Widget implementation. We separate the Widget from the
 * implementation because we don't want to expose internal methods.
 */
class EnttecUsbProWidgetImpl : public BaseUsbProWidget {
 public:
    EnttecUsbProWidgetImpl(
        ola::thread::SchedulerInterface *scheduler,
        ola::io::ConnectedDescriptor *descriptor,
        const EnttecUsbProWidget::EnttecUsbProWidgetOptions &options);
    ~EnttecUsbProWidgetImpl();

    void GetPortAssignments(
        EnttecUsbProWidget::EnttecUsbProPortAssignmentCallback *callback);
    void Stop();

    unsigned int PortCount() const { return m_ports.size(); }
    EnttecPort *GetPort(unsigned int i);

    bool SendCommand(uint8_t label, const uint8_t *data, unsigned int length);

 private:
    typedef vector<EnttecUsbProWidget::EnttecUsbProPortAssignmentCallback*>
      PortAssignmentCallbacks;

    ola::thread::SchedulerInterface *m_scheduler;
    ola::thread::timeout_id m_watchdog_timer_id;

    vector<EnttecPort*> m_ports;
    vector<EnttecPortImpl*> m_port_impls;
    auto_ptr<EnttecPortImpl::SendCallback> m_send_cb;
    UID m_uid;
    PortAssignmentCallbacks m_port_assignment_callbacks;

    // We override handle message to catch the messages, and dispatch them to
    // the correct port.
    void HandleMessage(uint8_t label, const uint8_t *data, unsigned int length);
    void HandleLabel(EnttecPortImpl *port, const OperationLabels &ops,
                     uint8_t label, const uint8_t *data, unsigned int length);
    void HandlePortAssignment(const uint8_t *data, unsigned int length);
    void AddPort(const OperationLabels &ops, unsigned int queue_size,
                 bool enable_rdm, bool no_rdm_dub_timeout = false);
    void EnableSecondPort();

    bool Watchdog();

    static const uint8_t PORT_ASSIGNMENT_LABEL = 141;
    static const uint8_t SET_PORT_ASSIGNMENT_LABEL = 145;
};


/*
 * New Enttec Usb Pro Device.
 * This also works for the RDM Pro with the standard firmware loaded.
 */
EnttecUsbProWidgetImpl::EnttecUsbProWidgetImpl(
  ola::thread::SchedulerInterface *scheduler,
  ola::io::ConnectedDescriptor *descriptor,
  const EnttecUsbProWidget::EnttecUsbProWidgetOptions &options)
    : BaseUsbProWidget(descriptor),
      m_scheduler(scheduler),
      m_watchdog_timer_id(ola::thread::INVALID_TIMEOUT),
      m_send_cb(NewCallback(this, &EnttecUsbProWidgetImpl::SendCommand)),
      m_uid(options.esta_id ? options.esta_id :
                              EnttecUsbProWidget::ENTTEC_ESTA_ID,
            options.serial) {
  AddPort(OperationLabels::Port1Operations(), options.queue_size,
          options.enable_rdm, options.no_rdm_dub_timeout);

  if (options.dual_ports) {
    AddPort(OperationLabels::Port2Operations(), options.queue_size,
            options.enable_rdm, options.no_rdm_dub_timeout);
    EnableSecondPort();
  }
  m_watchdog_timer_id = m_scheduler->RegisterRepeatingTimeout(
    TimeInterval(1, 0),
    NewCallback(this, &EnttecUsbProWidgetImpl::Watchdog));
}


EnttecUsbProWidgetImpl::~EnttecUsbProWidgetImpl() {
  Stop();
  STLDeleteElements(&m_ports);
  STLDeleteElements(&m_port_impls);
}


void EnttecUsbProWidgetImpl::GetPortAssignments(
    EnttecUsbProWidget::EnttecUsbProPortAssignmentCallback *callback) {
  if (m_ports.size() == 1) {
    // fake a response
    callback->Run(true, 1, 0);
    return;
  }

  m_port_assignment_callbacks.push_back(callback);
  if (!SendCommand(PORT_ASSIGNMENT_LABEL, NULL, 0)) {
    callback->Run(false, 1, 0);
  }
}


/**
 * Stop this widget
 */
void EnttecUsbProWidgetImpl::Stop() {
  if (m_watchdog_timer_id != ola::thread::INVALID_TIMEOUT) {
    m_scheduler->RemoveTimeout(m_watchdog_timer_id);
    m_watchdog_timer_id = ola::thread::INVALID_TIMEOUT;
  }

  vector<EnttecPortImpl*>::iterator iter = m_port_impls.begin();
  for (; iter != m_port_impls.end(); ++iter) {
    (*iter)->Stop();
  }

  PortAssignmentCallbacks::iterator cb_iter =
    m_port_assignment_callbacks.begin();
  for (; cb_iter != m_port_assignment_callbacks.end(); ++cb_iter) {
    (*cb_iter)->Run(false, 0, 0);
  }
  m_port_assignment_callbacks.clear();
}


/**
 * Given an index, return the EnttecPort
 */
EnttecPort *EnttecUsbProWidgetImpl::GetPort(unsigned int i) {
  if (i >= m_ports.size()) {
    return NULL;
  }
  return m_ports[i];
}


/**
 * Send a command to the widget
 */
bool EnttecUsbProWidgetImpl::SendCommand(uint8_t label, const uint8_t *data,
                                         unsigned int length) {
  OLA_DEBUG << "TX: " << IntToString(label) << ", length " << length;
  return SendMessage(label, data, length);
}


/*
 * Handle a message received from the widget
 */
void EnttecUsbProWidgetImpl::HandleMessage(uint8_t label,
                                           const uint8_t *data,
                                           unsigned int length) {
  OLA_DEBUG << "RX: " << IntToString(label) << ", length " << length;
  if (label == PORT_ASSIGNMENT_LABEL) {
    HandlePortAssignment(data, length);
  } else if (label > 128 && m_ports.size() > 1) {
    HandleLabel(m_port_impls[1], OperationLabels::Port2Operations(), label,
                data, length);
  } else {
    HandleLabel(m_port_impls[0], OperationLabels::Port1Operations(), label,
                data, length);
  }
}


void EnttecUsbProWidgetImpl::HandleLabel(EnttecPortImpl *port,
                                         const OperationLabels &ops,
                                         uint8_t label,
                                         const uint8_t *data,
                                         unsigned int length) {
  if (ops.get_params == label) {
    port->HandleParameters(data, length);
  } else if (ops.rdm_timeout == label) {
    port->HandleRDMTimeout(length);
  } else if (ops.recv_dmx == label) {
    port->HandleIncomingDataMessage(data, length);
  } else if (ops.cos_dmx == label) {
    port->HandleDMXDiff(data, length);
  } else {
      OLA_WARN << "Unknown message type " << ToHex(label) << ", length "
               << length;
  }
}


/**
 * Handle a port assignment message
 */
void EnttecUsbProWidgetImpl::HandlePortAssignment(const uint8_t *data,
                                                  unsigned int length) {
  bool ok = false;
  uint8_t port1_assignment = 0;
  uint8_t port2_assignment = 0;
  if (length == 2) {
    ok = true;
    port1_assignment = data[0];
    port2_assignment = data[1];
  }
  PortAssignmentCallbacks::iterator iter = m_port_assignment_callbacks.begin();
  for (; iter != m_port_assignment_callbacks.end(); ++iter) {
    (*iter)->Run(ok, port1_assignment, port2_assignment);
  }
  m_port_assignment_callbacks.clear();
}


/**
 * Add a port to this widget with the given operations.
 */
void EnttecUsbProWidgetImpl::AddPort(const OperationLabels &ops,
                                     unsigned int queue_size,
                                     bool enable_rdm,
                                     bool no_rdm_dub_timeout) {
  EnttecPortImpl *impl = new EnttecPortImpl(ops,
                                            m_uid,
                                            m_send_cb.get(),
                                            no_rdm_dub_timeout);
  m_port_impls.push_back(impl);
  EnttecPort *port = new EnttecPort(impl, queue_size, enable_rdm);
  m_ports.push_back(port);
}


void EnttecUsbProWidgetImpl::EnableSecondPort() {
  uint8_t data[] = {1, 1};
  if (!SendCommand(SET_PORT_ASSIGNMENT_LABEL, data, sizeof(data))) {
    OLA_INFO << "Failed to enable second port";
  }
}

bool EnttecUsbProWidgetImpl::Watchdog() {
  vector<EnttecPortImpl*>::iterator iter = m_port_impls.begin();
  for (; iter != m_port_impls.end(); ++iter) {
    (*iter)->ClockWatchdog();
  }
  return true;
}

// EnttecUsbProWidget
// ----------------------------------------------------------------------------

/**
 * EnttecUsbProWidget Constructor
 */
EnttecUsbProWidget::EnttecUsbProWidget(
    ola::thread::SchedulerInterface *scheduler,
    ola::io::ConnectedDescriptor *descriptor,
    const EnttecUsbProWidgetOptions &options) {
  m_impl = new EnttecUsbProWidgetImpl(scheduler, descriptor, options);
}


EnttecUsbProWidget::~EnttecUsbProWidget() {
  // delete the controller after the impl because the controller owns the
  // callback
  delete m_impl;
}


void EnttecUsbProWidget::GetPortAssignments(
    EnttecUsbProPortAssignmentCallback *callback) {
  m_impl->GetPortAssignments(callback);
}

void EnttecUsbProWidget::Stop() {
  m_impl->Stop();
}

unsigned int EnttecUsbProWidget::PortCount() const {
  return m_impl->PortCount();
}

EnttecPort *EnttecUsbProWidget::GetPort(unsigned int i) {
  return m_impl->GetPort(i);
}

ola::io::ConnectedDescriptor *EnttecUsbProWidget::GetDescriptor() const {
  return m_impl->GetDescriptor();
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
