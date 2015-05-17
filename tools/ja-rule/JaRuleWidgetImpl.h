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
 * JaRuleWidgetImpl.h
 * The implementation of the Ja Rule Widget.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef TOOLS_JA_RULE_JARULEWIDGETIMPL_H_
#define TOOLS_JA_RULE_JARULEWIDGETIMPL_H_

#include <ola/rdm/DiscoveryAgent.h>
#include <ola/rdm/QueueingRDMController.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/UID.h>
#include <ola/util/SequenceNumber.h>

#include "tools/ja-rule/JaRuleEndpoint.h"
#include "tools/ja-rule/JaRuleWidget.h"

class JaRuleWidgetImpl : public ola::rdm::DiscoveryTargetInterface,
                         public ola::rdm::DiscoverableRDMControllerInterface,
                         public JaRuleEndpoint::MessageHandlerInterface {
 public:
  JaRuleWidgetImpl(JaRuleEndpoint *device,
                   const ola::rdm::UID &controller_uid);

  ~JaRuleWidgetImpl();

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

  // From MessageHandlerInterface
  void NewMessage(const Message &message);

  /**
   * @brief Send a reset message to the hardware widget.
   */
  void ResetDevice();

 private:
  JaRuleEndpoint *m_device;
  ola::rdm::DiscoveryAgent m_discovery_agent;
  const ola::rdm::UID m_our_uid;
  ola::SequenceNumber<uint8_t> m_transaction_number;
  ola::rdm::RDMCallback *m_rdm_callback;
  MuteDeviceCallback *m_mute_callback;
  UnMuteDeviceCallback *m_unmute_callback;
  BranchCallback *m_branch_callback;
  ola::rdm::UIDSet m_uids;

  bool CheckForDevice() const;
  void PrintAck(const Message& message);
  void HandleDUBResponse(const Message& message);
  void HandleRDM(const Message& message);
  void DiscoveryComplete(ola::rdm::RDMDiscoveryCallback *callback,
                         OLA_UNUSED bool ok,
                         const ola::rdm::UIDSet &uids);

  DISALLOW_COPY_AND_ASSIGN(JaRuleWidgetImpl);
};

#endif  // TOOLS_JA_RULE_JARULEWIDGETIMPL_H_
