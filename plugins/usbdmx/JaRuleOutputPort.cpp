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
 * JaRuleOutputPort.cpp
 * A JaRule output port that uses a widget.
 * Copyright (C) 2015 Simon Newton
 */

#include "plugins/usbdmx/JaRuleOutputPort.h"

#include <string>

#include "libs/usb/JaRulePortHandle.h"
#include "ola/Logging.h"
#include "ola/strings/Format.h"
#include "olad/Device.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::usb::JaRulePortHandle;

JaRuleOutputPort::JaRuleOutputPort(Device *parent,
                                   unsigned int id,
                                   JaRulePortHandle *port_handle)
    : BasicOutputPort(parent, id, true, true),
      m_port_handle(port_handle) {
}

std::string JaRuleOutputPort::Description() const {
  return "Port " + ola::strings::IntToString(PortId() + 1);
}

bool JaRuleOutputPort::WriteDMX(const DmxBuffer &buffer,
                                OLA_UNUSED uint8_t priority) {
  m_port_handle->SendDMX(buffer);
  return true;
}

void JaRuleOutputPort::SendRDMRequest(ola::rdm::RDMRequest *request,
                                      ola::rdm::RDMCallback *callback) {
  m_port_handle->SendRDMRequest(request, callback);
}

void JaRuleOutputPort::RunFullDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  m_port_handle->RunFullDiscovery(callback);
}

void JaRuleOutputPort::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  m_port_handle->RunIncrementalDiscovery(callback);
}

bool JaRuleOutputPort::PreSetUniverse(Universe *old_universe,
                                      Universe *new_universe) {
  if (old_universe == NULL && new_universe != NULL) {
    m_port_handle->SetPortMode(ola::usb::CONTROLLER_MODE);
  }
  return true;
}

void JaRuleOutputPort::PostSetUniverse(Universe *old_universe,
                                       Universe *new_universe) {
  if (old_universe != NULL && new_universe == NULL) {
    m_port_handle->SetPortMode(ola::usb::RESPONDER_MODE);
  }
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
