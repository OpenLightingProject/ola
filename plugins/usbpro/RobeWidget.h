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
 * RobeWidget.h
 * Read and Write to a USB Widget.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_ROBEWIDGET_H_
#define PLUGINS_USBPRO_ROBEWIDGET_H_

#include <stdint.h>
#include <memory>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/io/Descriptor.h"
#include "ola/rdm/DiscoveryAgent.h"
#include "ola/rdm/QueueingRDMController.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/thread/SchedulingExecutorInterface.h"
#include "plugins/usbpro/BaseRobeWidget.h"

class RobeWidgetTest;


namespace ola {
namespace plugin {
namespace usbpro {

/*
 * A Robe USB Widget implementation.
 */
class RobeWidgetImpl: public BaseRobeWidget,
                      public ola::rdm::DiscoverableRDMControllerInterface,
                      public ola::rdm::DiscoveryTargetInterface {
 public:
    explicit RobeWidgetImpl(ola::io::ConnectedDescriptor *descriptor,
                            const ola::rdm::UID &uid);
    ~RobeWidgetImpl() {}

    void Stop();

    bool SendDMX(const DmxBuffer &buffer);

    void SendRDMRequest(ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete);
    void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
    void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);

    // incoming DMX methods
    bool ChangeToReceiveMode();
    // ownership of the callback is transferred
    void SetDmxCallback(Callback0<void> *callback);
    const DmxBuffer &FetchDMX() {
      return m_buffer;
    }

    // The following are the implementation of DiscoveryTargetInterface
    void MuteDevice(const ola::rdm::UID &target,
                    MuteDeviceCallback *mute_complete);
    void UnMuteAll(UnMuteDeviceCallback *unmute_complete);
    void Branch(const ola::rdm::UID &lower,
                const ola::rdm::UID &upper,
                BranchCallback *callback);

    static const int DMX_FRAME_DATA_SIZE;

 private:
    ola::rdm::RDMCallback *m_rdm_request_callback;
    MuteDeviceCallback *m_mute_callback;
    UnMuteDeviceCallback *m_unmute_callback;
    BranchCallback *m_branch_callback;
    ola::rdm::DiscoveryAgent m_discovery_agent;
    std::auto_ptr<Callback0<void> > m_dmx_callback;
    DmxBuffer m_buffer;
    std::auto_ptr<const ola::rdm::RDMRequest> m_pending_request;
    const ola::rdm::UID m_uid;
    uint8_t m_transaction_number;

    void HandleMessage(uint8_t label,
                       const uint8_t *data,
                       unsigned int length);
    void HandleRDMResponse(const uint8_t *data,
                           unsigned int length);
    void HandleDiscoveryResponse(const uint8_t *data,
                                 unsigned int length);
    void DiscoveryComplete(ola::rdm::RDMDiscoveryCallback *callback,
                           bool status,
                           const ola::rdm::UIDSet &uids);
    void HandleDmxFrame(const uint8_t *data, unsigned int length);
    bool PackAndSendRDMRequest(uint8_t label,
                               const ola::rdm::RDMRequest *request);
    static const unsigned int RDM_PADDING_BYTES = 4;
};


/*
 * A Robe Widget. This mostly just wraps the implementation.
 */
class RobeWidget: public SerialWidgetInterface,
                  public ola::rdm::DiscoverableRDMControllerInterface {
 public:
    RobeWidget(ola::io::ConnectedDescriptor *descriptor,
               const ola::rdm::UID &uid,
               unsigned int queue_size = 20);
    ~RobeWidget();

    void Stop() { m_impl->Stop(); }

    ola::io::ConnectedDescriptor *GetDescriptor() const {
      return m_impl->GetDescriptor();
    }

    bool SendDMX(const DmxBuffer &buffer) {
      return m_impl->SendDMX(buffer);
    }

    void SendRDMRequest(ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete) {
      m_controller->SendRDMRequest(request, on_complete);
    }

    void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      m_impl->RunFullDiscovery(callback);
    }

    void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      m_impl->RunIncrementalDiscovery(callback);
    }

    bool ChangeToReceiveMode() {
      return m_impl->ChangeToReceiveMode();
    }

    void SetDmxCallback(Callback0<void> *callback) {
      m_impl->SetDmxCallback(callback);
    }

    const DmxBuffer& FetchDMX() {
      return m_impl->FetchDMX();
    }

    // the tests access the implementation directly.
    friend class ::RobeWidgetTest;

 private:
    // we need to control the order of construction & destruction here so these
    // are pointers.
    RobeWidgetImpl *m_impl;
    ola::rdm::DiscoverableQueueingRDMController *m_controller;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_ROBEWIDGET_H_
