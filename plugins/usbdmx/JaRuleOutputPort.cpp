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

#include "olad/Device.h"
#include "plugins/usbdmx/JaRuleWidget.h"

namespace ola {
namespace plugin {
namespace usbdmx {

JaRuleOutputPort::JaRuleOutputPort(Device *parent,
                                   unsigned int id,
                                   JaRuleWidget *widget)
    : BasicOutputPort(parent, id, true),
      m_widget(widget) {
}

bool JaRuleOutputPort::WriteDMX(const DmxBuffer &buffer,
                                OLA_UNUSED uint8_t priority) {
  m_widget->SendDMX(buffer);
  return true;
}

void JaRuleOutputPort::SendRDMRequest(ola::rdm::RDMRequest *request,
                                      ola::rdm::RDMCallback *callback) {
  m_widget->SendRDMRequest(request, callback);
}

void JaRuleOutputPort::RunFullDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  m_widget->RunFullDiscovery(callback);
}

void JaRuleOutputPort::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  m_widget->RunIncrementalDiscovery(callback);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
