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
 * JaRulePortHandle.cpp
 * A Ja Rule Port Handle.
 * Copyright (C) 2015 Simon Newton
 */

#include "libs/usb/JaRulePortHandle.h"

#include <stdint.h>

#include <ola/DmxBuffer.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMControllerInterface.h>

#include <memory>

#include "libs/usb/JaRulePortHandleImpl.h"

namespace ola {
namespace usb {

JaRulePortHandle::JaRulePortHandle(class JaRuleWidgetPort *parent_port,
                                   const ola::rdm::UID &uid,
                                   uint8_t physical_port)
  : m_impl(new JaRulePortHandleImpl(parent_port, uid, physical_port)),
    m_queueing_controller(m_impl.get(), RDM_QUEUE_SIZE) {
}

JaRulePortHandle::~JaRulePortHandle() {
  // Pause the queuing controller so it stops sending anything more to the
  // impl.
  m_queueing_controller.Pause();
  // This will run any remaining callbacks.
  m_impl.reset();
  // m_queueing_controller will be destroyed next.
}

void JaRulePortHandle::SendRDMRequest(ola::rdm::RDMRequest *request,
                                      ola::rdm::RDMCallback *on_complete) {
  m_queueing_controller.SendRDMRequest(request, on_complete);
}

void JaRulePortHandle::RunFullDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  m_queueing_controller.RunFullDiscovery(callback);
}

void JaRulePortHandle::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  m_queueing_controller.RunIncrementalDiscovery(callback);
}

bool JaRulePortHandle::SendDMX(const DmxBuffer &buffer) {
  return m_impl->SendDMX(buffer);
}

bool JaRulePortHandle::SetPortMode(JaRulePortMode new_mode) {
  return m_impl->SetPortMode(new_mode);
}
}  // namespace usb
}  // namespace ola
