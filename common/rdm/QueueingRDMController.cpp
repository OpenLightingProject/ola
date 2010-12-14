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
 * QueueingRDMController.h
 * The Jese DMX TRI device.
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <string>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/rdm/QueueingRDMController.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/UsbWidget.h"

namespace ola {
namespace rdm {

using std::string;
using ola::rdm::RDMCommand;
using ola::rdm::RDMRequest;
using ola::rdm::UID;
using ola::rdm::UIDSet;


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
    m_response(NULL) {
  m_callback = ola::NewCallback(this,
                                &QueueingRDMController::HandleRDMResponse);
}


/*
 * Shutdown
 */
QueueingRDMController::~QueueingRDMController() {
  // delete all outstanding requests
  while (!m_pending_requests.empty()) {
    outstanding_rdm_request outstanding_request = m_pending_requests.front();
    if (outstanding_request.on_complete)
      outstanding_request.on_complete->Run(RDM_FAILED_TO_SEND, NULL);
    delete outstanding_request.request;
    m_pending_requests.pop();
  }

  if (m_response)
    delete m_response;

  if (m_callback)
    delete m_callback;
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
void QueueingRDMController::SendRDMRequest(const RDMRequest *request,
                                           RDMCallback *on_complete) {
  if (m_pending_requests.size() >= m_max_queue_size) {
    OLA_WARN << "RDM Queue is full, dropping request";
    if (on_complete)
      on_complete->Run(RDM_FAILED_TO_SEND, NULL);
    delete request;
    return;
  }

  outstanding_rdm_request outstanding_request;
  outstanding_request.request = request;
  outstanding_request.on_complete = on_complete;
  m_pending_requests.push(outstanding_request);
  MaybeSendRDMRequest();
}


/*
 * If we're not paused, send the next request.
 */
void QueueingRDMController::MaybeSendRDMRequest() {
  if (!m_active || m_pending_requests.empty() || m_rdm_request_pending)
    return;

  m_rdm_request_pending = true;
  DispatchNextRequest();
}


/*
 * Send the next RDM request, this assumes that SetFilter has been called
 */
void QueueingRDMController::DispatchNextRequest() {
  outstanding_rdm_request outstanding_request = m_pending_requests.front();
  // We have to make a copy here because we pass ownership of the request to
  // the underlying controller.
  m_controller->SendRDMRequest(outstanding_request.request->Duplicate(),
                            m_callback);
}


/*
 * Handle the response to a RemoteGet command
 */
void QueueingRDMController::HandleRDMResponse(
    rdm_response_status status,
    const ola::rdm::RDMResponse *response) {
  m_rdm_request_pending = false;

  if (m_pending_requests.empty()) {
    OLA_FATAL << "Recieved a response but the queue was empty!";
    return;
  }

  if (status == RDM_COMPLETED_OK && response == NULL) {
    // this is invalid, the only option here is to fail it
    OLA_FATAL << "State was OK but response is NULL!";
    status = RDM_INVALID_RESPONSE;
  } else if (status == RDM_COMPLETED_OK) {
    uint8_t original_type = response->ResponseType();
    if (m_response) {
      // if this is part of an overflowed response we need to combine it
      RDMResponse *combined_response =
        RDMResponse::CombineResponses(m_response, response);
      delete m_response;
      delete response;
      m_response = combined_response;
    } else {
      m_response = response;
    }

    // m_response now points to a valid response, or null if we couldn't
    // combine correctly.
    if (m_response) {
      if (original_type == ACK_OVERFLOW) {
        // send the same command again;
        DispatchNextRequest();
        return;
      }
    } else {
      status = RDM_INVALID_RESPONSE;
    }
  } else {
    // If an error occurs mid-transaction we abort it.
    if (m_response)
      delete m_response;
    m_response = NULL;
  }
  outstanding_rdm_request outstanding_request = m_pending_requests.front();
  if (outstanding_request.on_complete)
    outstanding_request.on_complete->Run(status, m_response);
  m_response = NULL;
  delete outstanding_request.request;
  m_pending_requests.pop();
  MaybeSendRDMRequest();
}
}  // rdm
}  // ola
