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
 * DmxterWidget.h
 * The Goddard Design Dmxter RDM and miniDmxter
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_DMXTERWIDGET_H_
#define PLUGINS_USBPRO_DMXTERWIDGET_H_

#include "ola/network/SelectServerInterface.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/QueueingRDMController.h"
#include "plugins/usbpro/UsbWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * A DMXter Widget implementation. We separate the Widget from the
 * implementation so we can leverage the QueueingRDMController.
 */
class DmxterWidgetImpl: public ola::rdm::RDMControllerInterface {
  public:
    DmxterWidgetImpl(ola::network::SelectServerInterface *ss,
                     UsbWidgetInterface *widget,
                     uint16_t esta_id,
                     uint32_t serial);
    ~DmxterWidgetImpl();

    void SetUIDListCallback(
        ola::Callback1<void, const ola::rdm::UIDSet&> *callback);

    void HandleMessage(uint8_t label,
                       unsigned int length,
                       const uint8_t *data);

    void SendRequest(const ola::rdm::RDMRequest *request,
                     ola::rdm::RDMCallback *on_complete);

    void RunRDMDiscovery();
    void SendUIDUpdate();
    void SendTodRequest();

  private:
    ola::rdm::UID m_uid;
    UsbWidgetInterface *m_widget;
    ola::network::SelectServerInterface *m_ss;
    ola::rdm::UIDSet m_uids;
    ola::network::timeout_id m_rdm_timeout;
    ola::Callback1<void, const ola::rdm::UIDSet&> *m_uid_set_callback;
    const ola::rdm::RDMRequest *m_rdm_request;
    ola::rdm::RDMCallback *m_rdm_request_callback;
    uint8_t m_transaction_number;

    void HandleTodResponse(const uint8_t *data, unsigned int length);
    void HandleRDMResponse(const uint8_t *data, unsigned int length);

    static const uint8_t RDM_REQUEST_LABEL;
    static const uint8_t RDM_BCAST_REQUEST_LABEL;
    static const uint8_t TOD_LABEL;
    static const uint8_t DISCOVERY_BRANCH_LABEL;
    // add the shutdown msg here
};


/*
 * A DMXter Widget. This mostly just wraps the implementation.
 */
class DmxterWidget: public ola::rdm::RDMControllerInterface {
  public:
    DmxterWidget(ola::network::SelectServerInterface *ss,
                 UsbWidgetInterface *widget,
                 uint16_t esta_id,
                 uint32_t serial,
                 unsigned int queue_size = 20);
    ~DmxterWidget() {}

    void SetUIDListCallback(
        ola::Callback1<void, const ola::rdm::UIDSet&> *callback) {
      m_impl.SetUIDListCallback(callback);
    }

    void SendRequest(const ola::rdm::RDMRequest *request,
                     ola::rdm::RDMCallback *on_complete) {
      m_controller.SendRequest(request, on_complete);
    }

    void RunRDMDiscovery() {
      return m_impl.RunRDMDiscovery();
    }

    void SendUIDUpdate() {
      return m_impl.SendUIDUpdate();
    }

    void SendTodRequest() {
      return m_impl.SendTodRequest();
    }

  private:
    DmxterWidgetImpl m_impl;
    ola::rdm::QueueingRDMController m_controller;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_DMXTERWIDGET_H_
