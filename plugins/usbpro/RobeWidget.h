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
 * RobeWidget.h
 * Read and Write to a USB Widget.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_ROBEWIDGET_H_
#define PLUGINS_USBPRO_ROBEWIDGET_H_

#include <stdint.h>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/network/Socket.h"
#include "ola/rdm/QueueingRDMController.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/thread/SchedulingExecutorInterface.h"
#include "plugins/usbpro/BaseRobeWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * A Robe USB Widget implementation.
 */
class RobeWidgetImpl: public BaseRobeWidget,
                      public ola::rdm::DiscoverableRDMControllerInterface {
  public:
    explicit RobeWidgetImpl(ola::network::ConnectedDescriptor *descriptor,
                            ola::thread::SchedulingExecutorInterface *ss,
                            const ola::rdm::UID &uid);
    ~RobeWidgetImpl() {}

    bool SendDMX(const DmxBuffer &buffer);

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete);
    bool RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
    bool RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);

    static const int DMX_FRAME_DATA_SIZE;

  private:
    ola::rdm::UIDSet m_uids;
    ola::thread::SchedulingExecutorInterface *m_ss;
    ola::rdm::RDMCallback *m_rdm_request_callback;
    const ola::rdm::RDMRequest *m_pending_request;
    const ola::rdm::UID m_uid;
    uint8_t m_transaction_number;
    ola::thread::timeout_id m_timeout_id;

    void HandleMessage(uint8_t label,
                       const uint8_t *data,
                       unsigned int length);
    void HandleRDMResponse(const uint8_t *data,
                           unsigned int length);
    void TimeoutRequest();

    static const unsigned int RDM_REQUEST_PADDING_BYTES = 4;
    // This should be 2.8ms but scheduling makes things difficult.
    static const unsigned int RDM_TIMEOUT_MS = 30;
};


/*
 * A Robe Widget. This mostly just wraps the implementation.
 */
class RobeWidget: public SerialWidgetInterface,
                  public ola::rdm::DiscoverableRDMControllerInterface {
  public:
    RobeWidget(ola::network::ConnectedDescriptor *descriptor,
               ola::thread::SchedulingExecutorInterface *ss,
               const ola::rdm::UID &uid,
               unsigned int queue_size = 20);
    ~RobeWidget();

    ola::network::ConnectedDescriptor *GetDescriptor() const {
      return m_impl->GetDescriptor();
    }

    bool SendDMX(const DmxBuffer &buffer) {
      return m_impl->SendDMX(buffer);
    }

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete) {
      m_controller->SendRDMRequest(request, on_complete);
    }

    bool RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      return m_impl->RunFullDiscovery(callback);
    }

    bool RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      return m_impl->RunIncrementalDiscovery(callback);
    }

  private:
    // we need to control the order of construction & destruction here so these
    // are pointers.
    RobeWidgetImpl *m_impl;
    ola::rdm::DiscoverableQueueingRDMController *m_controller;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_ROBEWIDGET_H_
