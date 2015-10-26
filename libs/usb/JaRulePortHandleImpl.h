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
 * JaRulePortHandleImpl.h
 * The implementation of the Ja Rule Port Handle.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef LIBS_USB_JARULEPORTHANDLEIMPL_H_
#define LIBS_USB_JARULEPORTHANDLEIMPL_H_

#include <ola/base/Macro.h>
#include <ola/DmxBuffer.h>
#include <ola/io/ByteString.h>
#include <ola/rdm/DiscoveryAgent.h>
#include <ola/rdm/QueueingRDMController.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/UID.h>
#include <ola/util/SequenceNumber.h>

#include "libs/usb/JaRuleConstants.h"

namespace ola {
namespace usb {

/**
 * @brief The internal implementation of a Ja Rule Port Handle.
 *
 * This class is responsible for translating high level intent, e.g. "send RDM
 * command" to the binary messages that can then be passed to a JaRuleWidget.
 */
class JaRulePortHandleImpl
    : public ola::rdm::DiscoveryTargetInterface,
      public ola::rdm::DiscoverableRDMControllerInterface {
 public:
  /**
   * @brief Create a new JaRulePortHandleImpl.
   */
  JaRulePortHandleImpl(class JaRuleWidgetPort *parent_port,
                       const ola::rdm::UID &uid,
                       uint8_t physical_port);

  ~JaRulePortHandleImpl();

  // From DiscoverableRDMControllerInterface
  void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
  void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
  void SendRDMRequest(ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *on_complete);

  // From DiscoveryTargetInterface
  void MuteDevice(const ola::rdm::UID &target,
                  MuteDeviceCallback *mute_complete);
  void UnMuteAll(UnMuteDeviceCallback *unmute_complete);
  void Branch(const ola::rdm::UID &lower,
              const ola::rdm::UID &upper,
              BranchCallback *branch_complete);

  /**
   * @brief Send DMX data from this widget
   * @param buffer The DmxBuffer containing the data to send.
   * @returns true if the data was sent, false otherwise.
   */
  bool SendDMX(const DmxBuffer &buffer);

  /**
   * @brief Change the mode of this port.
   */
  bool SetPortMode(JaRulePortMode new_mode);

 private:
  PACK(
  struct DUBTiming {
    uint16_t start;  //!< The start of the discovery response.
    uint16_t end;  //!< The end of the discovery response.
  });

  PACK(
  struct GetSetTiming {
    uint16_t break_start;  //!< The start of the break.
    uint16_t mark_start;  //!< The start of the mark / end of the break.
    uint16_t mark_end;  //!< The end of the mark.
  });

  class JaRuleWidgetPort* const m_port;  // not owned
  const ola::rdm::UID m_uid;
  const uint8_t m_physical_port;

  bool m_in_shutdown;

  // DMX members
  DmxBuffer m_dmx;
  bool m_dmx_in_progress;
  bool m_dmx_queued;
  CommandCompleteCallback *m_dmx_callback;

  // RDM members
  ola::rdm::DiscoveryAgent m_discovery_agent;
  ola::SequenceNumber<uint8_t> m_transaction_number;
  ola::rdm::UIDSet m_uids;

  void CheckStatusFlags(uint8_t flags);
  void DMXComplete(USBCommandResult result, JaRuleReturnCode return_code,
                   uint8_t status_flags, const ola::io::ByteString &payload);
  void MuteDeviceComplete(MuteDeviceCallback *mute_complete,
                          USBCommandResult result,
                          JaRuleReturnCode return_code,
                          uint8_t status_flags,
                          const ola::io::ByteString &payload);
  void UnMuteDeviceComplete(UnMuteDeviceCallback *unmute_complete,
                            USBCommandResult result,
                            JaRuleReturnCode return_code,
                            uint8_t status_flags,
                            const ola::io::ByteString &payload);
  void DUBComplete(BranchCallback *callback,
                   USBCommandResult status,
                   JaRuleReturnCode return_code,
                   uint8_t status_flags,
                   const ola::io::ByteString &payload);
  void RDMComplete(const ola::rdm::RDMRequest *request,
                   ola::rdm::RDMCallback *m_rdm_callback,
                   USBCommandResult result,
                   JaRuleReturnCode return_code,
                   uint8_t status_flags,
                   const ola::io::ByteString &payload);
  ola::rdm::RDMResponse* UnpackRDMResponse(
      const ola::rdm::RDMRequest *request,
      const ola::io::ByteString &payload,
      ola::rdm::RDMStatusCode *status_code);
  void DiscoveryComplete(ola::rdm::RDMDiscoveryCallback *callback,
                         OLA_UNUSED bool ok,
                         const ola::rdm::UIDSet &uids);
  CommandClass GetCommandFromRequest(const ola::rdm::RDMRequest *request);

  DISALLOW_COPY_AND_ASSIGN(JaRulePortHandleImpl);
};
}  // namespace usb
}  // namespace ola
#endif  // LIBS_USB_JARULEPORTHANDLEIMPL_H_
