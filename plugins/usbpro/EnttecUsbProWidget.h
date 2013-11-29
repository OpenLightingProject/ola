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


class EnttecPortImpl;

/**
 * A port represents a universe of DMX. It can be used to either send or
 * receive DMX.
 */
class EnttecPort
    : public ola::rdm::DiscoverableRDMControllerInterface {
 public:
    // Ownership not transferred.
    EnttecPort(EnttecPortImpl *impl, unsigned int queue_size);
    ~EnttecPort();

    bool SendDMX(const DmxBuffer &buffer);
    const DmxBuffer &FetchDMX() const;
    void SetDMXCallback(ola::Callback0<void> *callback);
    bool ChangeToReceiveMode(bool change_only);
    void GetParameters(usb_pro_params_callback *callback);
    bool SetParameters(uint8_t break_time, uint8_t mab_time, uint8_t rate);

    // the following are from DiscoverableRDMControllerInterface
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

    // the tests access the implementation directly.
    friend class ::EnttecUsbProWidgetTest;

 private:
    EnttecPortImpl *m_impl;
    ola::rdm::DiscoverableQueueingRDMController *m_controller;
};


/*
 * An Enttec Usb Pro Widget
 */
class EnttecUsbProWidget: public SerialWidgetInterface {
 public:
    /*
     * The callback to run when we receive a port assignment response
     * @param true if this command completed ok
     * @param DMX port 1 assignment
     * @param DMX port 2 assignment
     */
    typedef ola::SingleUseCallback3<void, bool, uint8_t, uint8_t>
      EnttecUsbProPortAssignmentCallback;

    struct EnttecUsbProWidgetOptions {
      uint16_t esta_id;
      uint32_t serial;
      bool dual_ports;
      unsigned int queue_size;

      EnttecUsbProWidgetOptions()
          : esta_id(0),
            serial(0),
            dual_ports(false),
            queue_size(20) {
      }

      EnttecUsbProWidgetOptions(uint16_t esta_id, uint32_t serial)
          : esta_id(esta_id),
            serial(serial),
            dual_ports(false),
            queue_size(20) {
      }
    };

    EnttecUsbProWidget(ola::io::ConnectedDescriptor *descriptor,
                       const EnttecUsbProWidgetOptions &options);
    ~EnttecUsbProWidget();

    void GetPortAssignments(EnttecUsbProPortAssignmentCallback *callback);

    void Stop();
    unsigned int PortCount() const;
    EnttecPort *GetPort(unsigned int i);
    ola::io::ConnectedDescriptor *GetDescriptor() const;

    static const uint16_t ENTTEC_ESTA_ID;

 private:
    class EnttecUsbProWidgetImpl *m_impl;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_ENTTECUSBPROWIDGET_H_
