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

#ifndef PLUGINS_USBPRO_ENTTECUSBPROWIDGET_H_
#define PLUGINS_USBPRO_ENTTECUSBPROWIDGET_H_

#include <deque>
#include <string>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/thread/SchedulerInterface.h"
#include "ola/rdm/DiscoveryAgent.h"
#include "ola/rdm/QueueingRDMController.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/GenericUsbProWidget.h"

class EnttecUsbProWidgetTest;

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * An Enttec DMX USB PRO Widget implementation. We separate the Widget from the
 * implementation so we can use the QueueingRDMController.
 */
class EnttecUsbProWidgetImpl
    : public GenericUsbProWidget,
      public ola::rdm::DiscoverableRDMControllerInterface,
      public ola::rdm::DiscoveryTargetInterface {
  public:
    EnttecUsbProWidgetImpl(ola::thread::SchedulerInterface *scheduler,
                           ola::io::ConnectedDescriptor *descriptor,
                           uint16_t esta_id,
                           uint32_t serial);
    ~EnttecUsbProWidgetImpl() { Stop(); }

    void Stop();

    // the following are from DiscoverableRDMControllerInterface
    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete);
    void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
    void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);

    // The following are the implementation of DiscoveryTargetInterface
    void MuteDevice(const ola::rdm::UID &target,
                    MuteDeviceCallback *mute_complete);
    void UnMuteAll(UnMuteDeviceCallback *unmute_complete);
    void Branch(const ola::rdm::UID &lower,
                const ola::rdm::UID &upper,
                BranchCallback *callback);

    // We override handle message to catch the incomming RDM frames
    void HandleMessage(uint8_t label,
                       const uint8_t *data,
                       unsigned int length);

  private:
    ola::rdm::DiscoveryAgent m_discovery_agent;
    const ola::rdm::UID m_uid;
    uint8_t m_transaction_number;
    ola::rdm::RDMCallback *m_rdm_request_callback;
    MuteDeviceCallback *m_mute_callback;
    UnMuteDeviceCallback *m_unmute_callback;
    BranchCallback *m_branch_callback;
    const ola::rdm::RDMRequest *m_pending_request;
    // holds the discovery response while we're waiting for the timeout message
    const uint8_t *m_discovery_response;
    unsigned int m_discovery_response_size;

    void HandleRDMTimeout(unsigned int length);
    void HandleIncommingDataMessage(const uint8_t *data, unsigned int length);
    void DiscoveryComplete(ola::rdm::RDMDiscoveryCallback *callback,
                           bool status,
                           const ola::rdm::UIDSet &uids);
    bool PackAndSendRDMRequest(uint8_t label,
                               const ola::rdm::RDMRequest *request);
    bool IsDUBRequest(const ola::rdm::RDMRequest *request);

    static const uint8_t RDM_PACKET = 7;
    static const uint8_t RDM_TIMEOUT_PACKET = 12;
    static const uint8_t RDM_DISCOVERY_PACKET = 11;
};


/*
 * An Enttec Usb Pro Widget
 */
class EnttecUsbProWidget: public SerialWidgetInterface,
                          public ola::rdm::DiscoverableRDMControllerInterface {
  public:
    EnttecUsbProWidget(ola::thread::SchedulerInterface *scheduler,
                       ola::io::ConnectedDescriptor *descriptor,
                       uint16_t esta_id,
                       uint32_t serial,
                       unsigned int queue_size = 20);
    ~EnttecUsbProWidget();

    void Stop() { m_impl->Stop(); }

    bool SendDMX(const DmxBuffer &buffer) {
      return m_impl->SendDMX(buffer);
    }

    const DmxBuffer &FetchDMX() const {
      return m_impl->FetchDMX();
    }

    void SetDMXCallback(
        ola::Callback0<void> *callback) {
      m_impl->SetDMXCallback(callback);
    }

    bool ChangeToReceiveMode(bool change_only) {
      return m_impl->ChangeToReceiveMode(change_only);
    }

    void GetParameters(usb_pro_params_callback *callback) {
      m_impl->GetParameters(callback);
    }

    bool SetParameters(uint8_t break_time,
                       uint8_t mab_time,
                       uint8_t rate) {
      return m_impl->SetParameters(break_time, mab_time, rate);
    }

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
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

    static const uint16_t ENTTEC_ESTA_ID;

    // the tests access the implementation directly.
    friend class ::EnttecUsbProWidgetTest;

  private:
    // we need to control the order of construction & destruction here so these
    // are pointers.
    EnttecUsbProWidgetImpl *m_impl;
    ola::rdm::DiscoverableQueueingRDMController *m_controller;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_ENTTECUSBPROWIDGET_H_
