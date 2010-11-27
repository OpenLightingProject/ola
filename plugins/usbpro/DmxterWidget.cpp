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
 * DmxterWidgetImpl.h
 * The Goddard Design Dmxter RDM and miniDmxter
 * Copyright (C) 2010 Simon Newton
 */

#include <string>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/DmxterWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::rdm::RDMRequest;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::string;

const uint8_t DmxterWidgetImpl::RDM_REQUEST_LABEL = 0x80;
const uint8_t DmxterWidgetImpl::RDM_BCAST_REQUEST_LABEL = 0x81;
const uint8_t DmxterWidgetImpl::TOD_LABEL = 0x82;
const uint8_t DmxterWidgetImpl::DISCOVERY_BRANCH_LABEL = 0x83;


/*
 * New DMXter device
 * @param ss A pointer to a SelectServerInterface
 * @param widget the underlying UsbWidget
 * @param esta_id the ESTA id, should normally be GODDARD Design
 * @param serial the 4 byte serial which forms part of the UID
 */
DmxterWidgetImpl::DmxterWidgetImpl(ola::network::SelectServerInterface *ss,
                                   UsbWidgetInterface *widget,
                                   uint16_t esta_id,
                                   uint32_t serial):
    m_uid(esta_id, serial),
    m_widget(widget),
    m_ss(ss),
    m_rdm_timeout(ola::network::INVALID_TIMEOUT),
    m_uid_set_callback(NULL),
    m_rdm_request_callback(NULL),
    m_transaction_number(0) {
  m_widget->SetMessageHandler(
      NewCallback(this, &DmxterWidgetImpl::HandleMessage));
}


/**
 * Clean up
 */
DmxterWidgetImpl::~DmxterWidgetImpl() {
  if (m_rdm_timeout != ola::network::INVALID_TIMEOUT) {
    m_ss->RemoveTimeout(m_rdm_timeout);
    m_rdm_timeout = ola::network::INVALID_TIMEOUT;
  }

  // timeout any existing message
  if (m_rdm_request_callback)
    m_rdm_request_callback->Run(ola::rdm::RDM_TIMEOUT, NULL);

  if (m_uid_set_callback)
    delete m_uid_set_callback;
}


/**
 * Set the callback used when the UIDSet changes
 * @param callback the callback to run when a new UIDSet is available.
 */
void DmxterWidgetImpl::SetUIDListCallback(
    ola::Callback1<void, const ola::rdm::UIDSet&> *callback) {
  if (m_uid_set_callback)
    delete m_uid_set_callback;
  m_uid_set_callback = callback;
}


/**
 * Called when a new packet arrives
 */
void DmxterWidgetImpl::HandleMessage(uint8_t label,
                                     const uint8_t *data,
                                     unsigned int length) {
  OLA_INFO << "Got new packet: 0x" << std::hex <<
    static_cast<int>(label) <<
    ", size " << length;

  switch (label) {
    case TOD_LABEL:
      HandleTodResponse(data, length);
      break;
    case RDM_REQUEST_LABEL:
    case RDM_BCAST_REQUEST_LABEL:
      HandleRDMResponse(data, length);
      break;
    default:
      OLA_WARN << "Unknown label: 0x" << std::hex <<
        static_cast<int>(label);
  }
  return;
}


/**
 * Send an RDM request. By wrapping this in a QueueingRDMController, we ensure
 * that this is only called one-at-a-time.
 * @param request the RDMRequest object
 * @param on_complete the callback to run when the request completes or fails
 *
 * TODO(simon): Check the formatting for these and update the unittests
 */
void DmxterWidgetImpl::SendRequest(const RDMRequest *request,
                                   ola::rdm::RDMCallback *on_complete) {
  if (m_rdm_request_callback) {
    OLA_FATAL << "Previous request hasn't completed yet, dropping request";
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL);
    delete request;
    return;
  }

  m_rdm_request_callback = on_complete;
  unsigned int data_size = request->Size();  // add in the start code
  uint8_t *data = new uint8_t[data_size + 1];
  data[0] = ola::rdm::RDMCommand::START_CODE;

  bool r = request->PackWithControllerParams(data + 1,
                                             &data_size,
                                             m_uid,
                                             m_transaction_number,
                                             1);
  if (r) {
    uint8_t label = request->DestinationUID().IsBroadcast() ?
      RDM_BCAST_REQUEST_LABEL : RDM_REQUEST_LABEL;

    if (m_widget->SendMessage(label, data, data_size + 1)) {
      delete[] data;
      delete request;
      return;
    }
  } else {
    OLA_WARN << "Failed to pack message, dropping request";
  }
  on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL);
  m_rdm_request_callback = NULL;
  delete[] data;
  delete request;
}


/**
 * Trigger the RDM discovery for the widget.
 */
void DmxterWidgetImpl::RunRDMDiscovery() {
  // TODO(simon): maybe we should pause sending for this? Check with Eric.
}


/**
 * Run the UID Set callback with the current list of UIDs
 */
void DmxterWidgetImpl::SendUIDUpdate() {
  if (m_uid_set_callback)
    m_uid_set_callback->Run(m_uids);
}


/**
 * Send a TOD request to the widget
 */
void DmxterWidgetImpl::SendTodRequest() {
  m_widget->SendMessage(TOD_LABEL, NULL, 0);
  OLA_INFO << "Sent TOD request";
}


/**
 * Handle a TOD response
 */
void DmxterWidgetImpl::HandleTodResponse(const uint8_t *data,
                                         unsigned int length) {
  (void) data;
  if (length % UID::UID_SIZE) {
    OLA_WARN << "Response length " << length << " not divisible by " <<
      static_cast<int>(ola::rdm::UID::UID_SIZE) << ", ignoring packet";
    return;
  }

  m_uids.Clear();
  for (unsigned int i = 0; i < length; i+= UID::UID_SIZE) {
    UID uid(data + i);
    OLA_INFO << "added " << uid.ToString();
    m_uids.AddUID(uid);
  }
  SendUIDUpdate();
}


/**
 * Handle a RDM response.
 */
void DmxterWidgetImpl::HandleRDMResponse(const uint8_t *data,
                                         unsigned int length) {
  OLA_INFO << "got RDM response!";
  if (m_rdm_request_callback == NULL) {
    OLA_FATAL << "Got a response but no callback to run!";
    return;
  }

  (void) data;
  (void) length;
  // the format for this hasn't been decided yet. Just mark as a failure and
  // move on for now.
  m_rdm_request_callback->Run(ola::rdm::RDM_INVALID_RESPONSE, NULL);
  m_rdm_request_callback = NULL;
}


/**
 * DmxterWidget Constructor
 */
DmxterWidget::DmxterWidget(ola::network::SelectServerInterface *ss,
                           UsbWidgetInterface *widget,
                           uint16_t esta_id,
                           uint32_t serial,
                           unsigned int queue_size):
    m_impl(ss, widget, esta_id, serial),
    m_controller(&m_impl, queue_size) {
}
}  // usbpro
}  // plugin
}  // ola
