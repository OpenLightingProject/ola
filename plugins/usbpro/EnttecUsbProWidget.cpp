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
 * EnttecUsbProWidget.h
 * The Enttec USB Pro Widget
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/BaseUsbProWidget.h"
#include "plugins/usbpro/EnttecUsbProWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::rdm::RDMCommand;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMRequest;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::auto_ptr;


const uint16_t EnttecUsbProWidget::ENTTEC_ESTA_ID = 0x454E;
const uint8_t EnttecUsbProWidgetImpl::RDM_PACKET;
const uint8_t EnttecUsbProWidgetImpl::RDM_TIMEOUT_PACKET;
const uint8_t EnttecUsbProWidgetImpl::RDM_DISCOVERY_PACKET;

/*
 * New Enttec Usb Pro Device.
 * This also works for the RDM Pro with the standard firmware loaded.
 */
EnttecUsbProWidgetImpl::EnttecUsbProWidgetImpl(
  ola::thread::SchedulerInterface *scheduler,
  ola::io::ConnectedDescriptor *descriptor,
  uint16_t esta_id,
  uint32_t serial)
    : GenericUsbProWidget(scheduler, descriptor),
      m_discovery_agent(this),
      m_uid(esta_id ? esta_id : EnttecUsbProWidget::ENTTEC_ESTA_ID, serial),
      m_transaction_number(0),
      m_rdm_request_callback(NULL),
      m_mute_callback(NULL),
      m_unmute_callback(NULL),
      m_branch_callback(NULL),
      m_pending_request(NULL),
      m_discovery_response(NULL),
      m_discovery_response_size(0) {
}


/**
 * Stop this widget
 */
void EnttecUsbProWidgetImpl::Stop() {
  m_discovery_agent.Abort();
  GenericStop();
}


/**
 * Send an RDM Request.
 */
void EnttecUsbProWidgetImpl::SendRDMRequest(
    const ola::rdm::RDMRequest *request,
    ola::rdm::RDMCallback *on_complete) {
  std::vector<string> packets;
  if (m_rdm_request_callback) {
    OLA_WARN << "Previous request hasn't completed yet, dropping request";
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    delete request;
    return;
  }

  // Prepare the buffer for the RDM data including the start code.
  unsigned int rdm_size = RDMCommandSerializer::RequiredSize(*request);
  uint8_t *data = new uint8_t[rdm_size + 1];
  data[0] = RDMCommand::START_CODE;

  unsigned int this_transaction_number = m_transaction_number++;
  unsigned int port_id = 1;

  bool r = RDMCommandSerializer::Pack(*request, &data[1], &rdm_size, m_uid,
                                      this_transaction_number, port_id);

  if (!r) {
    OLA_WARN << "Failed to pack message, dropping request";
    delete[] data;
    delete request;
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    return;
  }

  m_rdm_request_callback = on_complete;
  // re-write the request so it appears to originate from this widget.
  m_pending_request = request->DuplicateWithControllerParams(
      m_uid,
      this_transaction_number,
      port_id);

  const uint8_t label = (
      IsDUBRequest(request) ? RDM_DISCOVERY_PACKET : RDM_PACKET);
  delete request;
  if (!SendMessage(label, data, rdm_size + 1)) {
    m_rdm_request_callback = NULL;
    m_pending_request = NULL;
    delete[] data;
    delete request;
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    return;
  }
  delete[] data;
}


/**
 * Start full discovery for this widget.
 */
void EnttecUsbProWidgetImpl::RunFullDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  OLA_INFO << "Full discovery triggered";
  m_discovery_agent.StartFullDiscovery(
    ola::NewSingleCallback(this,
                           &EnttecUsbProWidgetImpl::DiscoveryComplete,
                           callback));
}


/**
 * Start incremental discovery for this widget
 */
void EnttecUsbProWidgetImpl::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  OLA_INFO << "Incremental discovery triggered";
  m_discovery_agent.StartIncrementalDiscovery(
    ola::NewSingleCallback(this,
                           &EnttecUsbProWidgetImpl::DiscoveryComplete,
                           callback));
}


/**
 * Mute a responder
 * @param target the UID to mute
 * @param MuteDeviceCallback the callback to run once the mute request
 * completes.
 */
void EnttecUsbProWidgetImpl::MuteDevice(const ola::rdm::UID &target,
                                        MuteDeviceCallback *mute_complete) {
  auto_ptr<RDMRequest> mute_request(
      ola::rdm::NewMuteRequest(m_uid, target, m_transaction_number++));
  OLA_INFO << "Muting " << target;
  if (PackAndSendRDMRequest(RDM_PACKET, mute_request.get()))
    m_mute_callback = mute_complete;
  else
    mute_complete->Run(false);
}


/**
 * Unmute all responders
 * @param UnMuteDeviceCallback the callback to run once the unmute request
 * completes.
 */
void EnttecUsbProWidgetImpl::UnMuteAll(UnMuteDeviceCallback *unmute_complete) {
  auto_ptr<RDMRequest> unmute_request(
      ola::rdm::NewUnMuteRequest(m_uid,
                                 ola::rdm::UID::AllDevices(),
                                 m_transaction_number++));
  OLA_INFO << "Un-muting all devices";
  if (PackAndSendRDMRequest(RDM_PACKET, unmute_request.get())) {
    m_unmute_callback = unmute_complete;
  } else {
    OLA_WARN << "Failed to send Unmute all request";
    unmute_complete->Run();
  }
}


/**
 * Send a Discovery Unique Branch
 */
void EnttecUsbProWidgetImpl::Branch(const ola::rdm::UID &lower,
                                    const ola::rdm::UID &upper,
                                    BranchCallback *callback) {
  auto_ptr<RDMRequest> branch_request(
      ola::rdm::NewDiscoveryUniqueBranchRequest(
          m_uid,
          lower,
          upper,
          m_transaction_number++));
  OLA_INFO << "Sending DUB packet: " << lower << " - " << upper;
  if (PackAndSendRDMRequest(RDM_DISCOVERY_PACKET, branch_request.get()))
    m_branch_callback = callback;
  else
    callback->Run(NULL, 0);
}


/*
 * Handle a message received from the widget
 */
void EnttecUsbProWidgetImpl::HandleMessage(uint8_t label,
                                           const uint8_t *data,
                                           unsigned int length) {
  switch (label) {
    case RDM_PACKET:
      // The widget isn't supposed to send frames with an id of 7, these are
      // just sent from the host to the widget.
      OLA_WARN <<
        "Enttec Pro received an RDM frame (id 7), this shouldn't happen.";
      break;
    case RDM_TIMEOUT_PACKET:
      HandleRDMTimeout(length);
      break;
    case GenericUsbProWidget::RECEIVED_DMX_LABEL:
      HandleIncommingDataMessage(data, length);
      break;
    default:
      GenericUsbProWidget::HandleMessage(label, data, length);
  }
}


/**
 * Called to indicate the completion of an RDM request.
 * According to the spec:
 *  The timeout message will follow the RDM discovery reply message, whether or
 *   not the reply is partial or complete.
 *  The timeout message will follow the RDM reply message (GET or SET), only
 *   when the reply is incomplete or unrecognizable.
 *
 * Experiments suggest that sending another RDM message before this 'timeout'
 * is received results in Bad Things Happening.
 *
 * The length of this message should be 0.
 */
void EnttecUsbProWidgetImpl::HandleRDMTimeout(unsigned int length) {
  if (length)
    OLA_WARN << "Strange RDM timeout message, length was " << length;

  // check what operation we were waiting on
  if (m_unmute_callback) {
    UnMuteDeviceCallback *callback = m_unmute_callback;
    m_unmute_callback = NULL;
    callback->Run();
  } else if (m_mute_callback) {
    MuteDeviceCallback *callback = m_mute_callback;
    m_mute_callback = NULL;
    OLA_INFO << "Failed to mute device";
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
  } else if (m_rdm_request_callback && m_pending_request) {
    ola::rdm::rdm_response_code code;
    if (IsDUBRequest(m_pending_request))
        code = ola::rdm::RDM_TIMEOUT;
    else
      code = (
          m_pending_request->DestinationUID().IsBroadcast() ?
          ola::rdm::RDM_WAS_BROADCAST :
          ola::rdm::RDM_TIMEOUT);

    ola::rdm::RDMCallback *callback = m_rdm_request_callback;
    m_rdm_request_callback = NULL;
    delete m_pending_request;
    m_pending_request = NULL;
    std::vector<std::string> packets;
    callback->Run(code, NULL, packets);
  }
}


/**
 * Handle an incomming frame.
 * @param data the incoming data buffer
 * @param length the length of the data buffer.
 *
 * The first byte is a status code: 0: good, non-0: bad
 * The second byte is the start code
 * The remaining bytes are the actual data.
 */
void EnttecUsbProWidgetImpl::HandleIncommingDataMessage(
    const uint8_t *data,
    unsigned int length) {
  bool waiting_for_dub_response = (
      m_branch_callback != NULL || (
      (m_rdm_request_callback && IsDUBRequest(m_pending_request))));

  // if we're not waiting for a DUB response, and this isn't an RDM frame, then
  // let the super class handle it.
  if (!waiting_for_dub_response && length >= 2 &&
      data[1] != ola::rdm::RDMCommand::START_CODE) {
    HandleDMX(data, length);
    return;
  }

  // It's not clear what happens if we get an overrun on an RDM response.
  // Do we still get the timeout message or is this the only response?
  // I need to check with Nic.
  if (data[0]) {
    OLA_WARN << "Incomming frame corrupted";
    return;
  }

  // skip over the status bit
  data++;
  length--;

  if (m_branch_callback) {
    // discovery responses are *always* followed by the timeout message and
    // it's important that we wait for this before sending the next command
    if (m_discovery_response) {
      OLA_WARN <<
        "multiple discovery responses received, ignoring all but the first.";
      return;
    }
    uint8_t *response_data = new uint8_t[length];
    memcpy(response_data, data, length);
    m_discovery_response = response_data;
    m_discovery_response_size = length;
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
    const ola::rdm::RDMRequest *request = m_pending_request;
    m_pending_request = NULL;

    std::vector<std::string> packets;
    ola::rdm::rdm_response_code response_code;
    ola::rdm::RDMResponse *response = NULL;

    if (waiting_for_dub_response) {
      response_code = ola::rdm::RDM_DUB_RESPONSE;
      packets.push_back(
          string(reinterpret_cast<const char*>(data), length));
    } else {
      // try to inflate
      string packet(reinterpret_cast<const char*>(data + 1), length - 1);
      packets.push_back(packet);
      response = ola::rdm::RDMResponse::InflateFromData(
          packet,
          &response_code,
          request);
    }
    callback->Run(response_code, response, packets);
    delete request;
  }
}


/**
 * Called when the discovery process finally completes
 * @param callback the callback passed to StartFullDiscovery or
 * StartIncrementalDiscovery that we should execute.
 * @param status true if discovery worked, false otherwise
 * @param uids the UIDSet of UIDs that were found.
 */
void EnttecUsbProWidgetImpl::DiscoveryComplete(
    ola::rdm::RDMDiscoveryCallback *callback,
    bool,
    const UIDSet &uids) {
  OLA_DEBUG << "Enttec Pro discovery complete: " << uids;
  if (callback)
    callback->Run(uids);
}


/**
 * Send a RDM request to the widget
 */
bool EnttecUsbProWidgetImpl::PackAndSendRDMRequest(uint8_t label,
                                                   const RDMRequest *request) {
  unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
  uint8_t data[rdm_length + 1];  // inc start code
  data[0] = RDMCommand::START_CODE;
  RDMCommandSerializer::Pack(*request, &data[1], &rdm_length);
  return SendMessage(label, data, rdm_length + 1);
}


/**
 * Return true if this is a Discovery Unique Branch request
 */
bool EnttecUsbProWidgetImpl::IsDUBRequest(
    const ola::rdm::RDMRequest *request) {
  return (request->CommandClass() == ola::rdm::RDMCommand::DISCOVER_COMMAND &&
          request->ParamId() == ola::rdm::PID_DISC_UNIQUE_BRANCH);
}


/**
 * EnttecUsbProWidget Constructor
 */
EnttecUsbProWidget::EnttecUsbProWidget(
    ola::thread::SchedulerInterface *scheduler,
    ola::io::ConnectedDescriptor *descriptor,
    uint16_t esta_id,
    uint32_t serial,
    unsigned int queue_size) {
  m_impl = new EnttecUsbProWidgetImpl(scheduler, descriptor, esta_id, serial);
  m_controller = new ola::rdm::DiscoverableQueueingRDMController(m_impl,
                                                                 queue_size);
}


EnttecUsbProWidget::~EnttecUsbProWidget() {
  // delete the controller after the impl because the controller owns the
  // callback
  delete m_impl;
  delete m_controller;
}
}  // usbpro
}  // plugin
}  // ola
