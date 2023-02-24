/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * QueueingRDMController.cpp
 * The Jese DMX TRI device.
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <string>
#include <utility>
#include <vector>
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/rdm/QueueingRDMController.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"

namespace ola {
namespace rdm {

using ola::rdm::RDMCommand;
using ola::rdm::RDMRequest;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::string;
using std::vector;


/*
 * A new QueueingRDMController. This takes another controller as a argument,
 * and ensures that we only send one request at a time.
 */
QueueingRDMController::QueueingRDMController(
    RDMControllerInterface *controller,
    unsigned int max_queue_size)
  : m_controller(controller),
    m_max_queue_size(max_queue_size),
    m_rdm_request_pending(false),
    m_active(true),
    m_callback(ola::NewCallback(this,
                                &QueueingRDMController::HandleRDMResponse)) {
}


/*
 * Shutdown
 */
QueueingRDMController::~QueueingRDMController() {
  // delete all outstanding requests
  while (!m_pending_requests.empty()) {
    outstanding_rdm_request outstanding_request = m_pending_requests.front();
    if (outstanding_request.on_complete) {
      RunRDMCallback(outstanding_request.on_complete, RDM_FAILED_TO_SEND);
    }
    delete outstanding_request.request;
    m_pending_requests.pop();
  }
}


/**
 * Pause the sending of RDM messages. This won't cancel any message in-flight.
 */
void QueueingRDMController::Pause() {
  m_active = false;
}


/**
 * Resume the sending of RDM requests.
 */
void QueueingRDMController::Resume() {
  m_active = true;
  MaybeSendRDMRequest();
}


/**
 * Queue an RDM request for sending.
 */
void QueueingRDMController::SendRDMRequest(RDMRequest *request,
                                           RDMCallback *on_complete) {
  if (m_pending_requests.size() >= m_max_queue_size) {
    OLA_WARN << "RDM Queue is full, dropping request";
    if (on_complete) {
      RunRDMCallback(on_complete, RDM_FAILED_TO_SEND);
    }
    delete request;
    return;
  }

  outstanding_rdm_request outstanding_request;
  outstanding_request.request = request;
  outstanding_request.on_complete = on_complete;
  m_pending_requests.push(outstanding_request);
  TakeNextAction();
}


/**
 * Do the next action.
 */
void QueueingRDMController::TakeNextAction() {
  if (CheckForBlockingCondition())
    return;

  MaybeSendRDMRequest();
}


/**
 * This method runs before we decide to send another request and allows sub
 * classes (like the DiscoverableQueueingRDMController) to insert other actions
 * into the queue.
 * @returns true if some other action is running, false otherwise.
 */
bool QueueingRDMController::CheckForBlockingCondition() {
  return !m_active || m_rdm_request_pending;
}


/*
 * If we're not paused, send the next request.
 */
void QueueingRDMController::MaybeSendRDMRequest() {
  if (m_pending_requests.empty())
    return;

  m_rdm_request_pending = true;
  DispatchNextRequest();
}


/*
 * Send the next RDM request.
 */
void QueueingRDMController::DispatchNextRequest() {
  outstanding_rdm_request outstanding_request = m_pending_requests.front();
  // We have to make a copy here because we pass ownership of the request to
  // the underlying controller.
  // We need to have the original request because we use it if we receive an
  // ACK_OVERFLOW.
  m_controller->SendRDMRequest(outstanding_request.request->Duplicate(),
                               m_callback.get());
}


/*
 * Handle the response to a RemoteGet command
 */
void QueueingRDMController::HandleRDMResponse(RDMReply *reply) {
  m_rdm_request_pending = false;

  if (m_pending_requests.empty()) {
    OLA_FATAL << "Received a response but the queue was empty!";
    return;
  }

  bool was_ack_overflow = reply->StatusCode() == RDM_COMPLETED_OK &&
                          reply->Response() &&
                          reply->Response()->ResponseType() == ACK_OVERFLOW;
  // Check for ACK_OVERFLOW
  if (m_response.get()) {
    if (reply->StatusCode() != RDM_COMPLETED_OK || reply->Response() == NULL) {
      // We failed part way through an ACK_OVERFLOW
      m_frames.insert(m_frames.end(), reply->Frames().begin(),
                      reply->Frames().end());
      RDMReply new_reply(reply->StatusCode(), NULL, m_frames);
      RunCallback(&new_reply);
      m_response.reset();
      m_frames.clear();
      TakeNextAction();
    } else {
      // Combine the data.
      m_response.reset(RDMResponse::CombineResponses(
          m_response.get(), reply->Response()));
      m_frames.insert(m_frames.end(), reply->Frames().begin(),
                      reply->Frames().end());

      if (!m_response.get()) {
        // The response was invalid
        RDMReply new_reply(RDM_INVALID_RESPONSE, NULL, m_frames);
        RunCallback(&new_reply);
        m_frames.clear();
        TakeNextAction();
      } else if (reply->Response()->ResponseType() != ACK_OVERFLOW) {
        RDMReply new_reply(RDM_COMPLETED_OK, m_response.release(), m_frames);
        RunCallback(&new_reply);
        m_response.reset();
        m_frames.clear();
        TakeNextAction();
      } else {
        DispatchNextRequest();
      }
      return;
    }
  } else if (was_ack_overflow) {
    // We're in an ACK_OVERFLOW sequence.
    m_frames.clear();
    m_response.reset(reply->Response()->Duplicate());
    m_frames.insert(m_frames.end(), reply->Frames().begin(),
                    reply->Frames().end());
    DispatchNextRequest();
  } else {
    // Just pass the RDMReply on.
    RunCallback(reply);
    TakeNextAction();
  }
}

void QueueingRDMController::RunCallback(RDMReply *reply) {
  outstanding_rdm_request outstanding_request = m_pending_requests.front();
  m_pending_requests.pop();
  if (outstanding_request.on_complete) {
    outstanding_request.on_complete->Run(reply);
  }
  delete outstanding_request.request;
}


/**
 * Constructor for the DiscoverableQueueingRDMController
 */
DiscoverableQueueingRDMController::DiscoverableQueueingRDMController(
        DiscoverableRDMControllerInterface *controller,
        unsigned int max_queue_size)
    : QueueingRDMController(controller, max_queue_size),
      m_discoverable_controller(controller) {
}


/**
 * Run the full RDM discovery routine. This will either run immediately or
 * after the current request completes.
 */
void DiscoverableQueueingRDMController::RunFullDiscovery(
    RDMDiscoveryCallback *callback) {
  GenericDiscovery(callback, true);
}


/**
 * Run the incremental RDM discovery routine. This will either run immediately
 * or after the current request completes.
 */
void DiscoverableQueueingRDMController::RunIncrementalDiscovery(
    RDMDiscoveryCallback *callback) {
  GenericDiscovery(callback, false);
}


/**
 * Override this so we can prioritize the discovery requests.
 */
void DiscoverableQueueingRDMController::TakeNextAction() {
  if (CheckForBlockingCondition())
    return;

  // prioritize discovery above RDM requests
  if (!m_pending_discovery_callbacks.empty())
    StartRDMDiscovery();
  else
    MaybeSendRDMRequest();
}


/**
 * Block if either a RDM request is pending, or another discovery process is
 * running.
 */
bool DiscoverableQueueingRDMController::CheckForBlockingCondition() {
  return (QueueingRDMController::CheckForBlockingCondition() ||
          !m_discovery_callbacks.empty());
}


/**
 * The generic discovery routine
 */
void DiscoverableQueueingRDMController::GenericDiscovery(
    RDMDiscoveryCallback *callback,
    bool full) {
  m_pending_discovery_callbacks.push_back(std::make_pair(full, callback));
  TakeNextAction();
}


/**
 * Run the rdm discovery routine for the underlying controller.
 * @pre m_pending_discovery_callbacks is not empty()
 */
void DiscoverableQueueingRDMController::StartRDMDiscovery() {
  bool full = false;
  m_discovery_callbacks.reserve(m_pending_discovery_callbacks.size());

  PendingDiscoveryCallbacks::iterator iter =
    m_pending_discovery_callbacks.begin();
  for (; iter != m_pending_discovery_callbacks.end(); iter++) {
    full |= iter->first;
    m_discovery_callbacks.push_back(iter->second);
  }
  m_pending_discovery_callbacks.clear();

  RDMDiscoveryCallback *callback = NewSingleCallback(
      this,
      &DiscoverableQueueingRDMController::DiscoveryComplete);

  if (full)
    m_discoverable_controller->RunFullDiscovery(callback);
  else
    m_discoverable_controller->RunIncrementalDiscovery(callback);
}


/**
 * Called when discovery completes
 */
void DiscoverableQueueingRDMController::DiscoveryComplete(
    const ola::rdm::UIDSet &uids) {
  DiscoveryCallbacks::iterator iter = m_discovery_callbacks.begin();
  for (; iter != m_discovery_callbacks.end(); ++iter) {
    if (*iter)
      (*iter)->Run(uids);
  }
  m_discovery_callbacks.clear();
  TakeNextAction();
}
}  // namespace rdm
}  // namespace ola
