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
 * EnttecUsbProWidgetImpl.h
 * The Enttec USB Pro Widget
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_USBPRO_ENTTECUSBPROWIDGETIMPL_H_
#define PLUGINS_USBPRO_ENTTECUSBPROWIDGETIMPL_H_

#include <deque>
#include <memory>
#include "ola/Callback.h"
#include "ola/Constants.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/util/Watchdog.h"

namespace ola {
namespace plugin {
namespace usbpro {

enum {
  // port 1 labels
  GET_PARAMS_1 = 3,
  SET_PARAMS_1 = 4,
  RECEIVED_DMX_LABEL_1 = 5,
  SEND_DMX_1 = 6,
  SEND_RDM_1 = 7,
  CHANGE_TO_RX_1 = 8,
  COS_DMX_1 = 9,
  RDM_DISCOVERY_1 = 11,
  RDM_TIMEOUT_1 = 12,

  // port 2 labels, these are tied to the OLA API key.
  GET_PARAMS_2 = 137,
  SET_PARAMS_2 = 180,
  RECEIVED_DMX_LABEL_2 = 156,
  SEND_DMX_2 = 135,
  SEND_RDM_2 = 236,
  CHANGE_TO_RX_2 = 197,
  COS_DMX_2 = 164,
  RDM_DISCOVERY_2 = 196,
  RDM_TIMEOUT_2 = 201,
};

// Maps operations to label values
struct OperationLabels {
  uint8_t get_params;
  uint8_t set_params;
  uint8_t recv_dmx;
  uint8_t send_dmx;
  uint8_t send_rdm;
  uint8_t change_to_rx_mode;
  uint8_t cos_dmx;
  uint8_t rdm_timeout;
  uint8_t rdm_discovery;

  static OperationLabels Port1Operations();
  static OperationLabels Port2Operations();
};


/**
 * The internal implementation of an Enttec port
 */
class EnttecPortImpl
    : public ola::rdm::DiscoverableRDMControllerInterface,
      public ola::rdm::DiscoveryTargetInterface {
 public:
    typedef ola::Callback3<bool, uint8_t, const uint8_t*, unsigned int>
      SendCallback;

    EnttecPortImpl(const OperationLabels &ops, const ola::rdm::UID &uid,
                   SendCallback *send_cb, bool no_rdm_dub_timeout = false);

    void Stop();

    bool SendDMX(const DmxBuffer &buffer);
    const DmxBuffer &FetchDMX() const { return m_input_buffer; }
    void SetDMXCallback(ola::Callback0<void> *callback);

    bool ChangeToReceiveMode(bool change_only);
    void GetParameters(usb_pro_params_callback *callback);
    bool SetParameters(uint8_t break_time, uint8_t mab_time, uint8_t rate);

    // the following are from DiscoverableRDMControllerInterface
    void SendRDMRequest(ola::rdm::RDMRequest *request,
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

    // Called by the EnttecUsbProWidgetImpl
    void HandleRDMTimeout(unsigned int length);
    void HandleParameters(const uint8_t *data, unsigned int length);
    void HandleIncomingDataMessage(const uint8_t *data, unsigned int length);
    void HandleDMXDiff(const uint8_t *data, unsigned int length);

    void ClockWatchdog();
    void WatchdogFired();

 private:
  SendCallback *m_send_cb;
  OperationLabels m_ops;
  bool m_active;
  Watchdog m_watchdog;

  // RX DMX
  DmxBuffer m_input_buffer;
  std::auto_ptr<ola::Callback0<void> > m_dmx_callback;

  // widget params
  std::deque<usb_pro_params_callback*> m_outstanding_param_callbacks;

  // RDM send
  ola::rdm::DiscoveryAgent m_discovery_agent;
  const ola::rdm::UID m_uid;
  uint8_t m_transaction_number;
  ola::rdm::RDMCallback *m_rdm_request_callback;
  std::auto_ptr<const ola::rdm::RDMRequest> m_pending_request;

  // RDM Discovery
  MuteDeviceCallback *m_mute_callback;
  UnMuteDeviceCallback *m_unmute_callback;
  BranchCallback *m_branch_callback;
  // holds the discovery response while we're waiting for the timeout message
  const uint8_t *m_discovery_response;
  unsigned int m_discovery_response_size;
  bool m_no_rdm_dub_timeout;

  void HandleDMX(const uint8_t *data, unsigned int length);
  void DiscoveryComplete(ola::rdm::RDMDiscoveryCallback *callback,
                         bool status,
                         const ola::rdm::UIDSet &uids);
  bool PackAndSendRDMRequest(uint8_t label,
                             const ola::rdm::RDMRequest *request);
  bool IsDUBRequest(const ola::rdm::RDMRequest *request);

  static const unsigned int PORT_ID = 1;
  // This gives a limit between 1 and 2s.
  static const unsigned int WATCHDOG_LIMIT = 2;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_ENTTECUSBPROWIDGETIMPL_H_
