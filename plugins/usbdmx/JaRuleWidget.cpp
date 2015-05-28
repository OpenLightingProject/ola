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
 * JaRuleWidget.cpp
 * A Ja Rule widget.
 * Copyright (C) 2015 Simon Newton
 */

#include <ola/Logging.h>
#include "plugins/usbdmx/JaRuleWidget.h"
#include "plugins/usbdmx/JaRuleWidgetImpl.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::rdm::RDMDiscoveryCallback;
using ola::rdm::RDMRequest;
using ola::rdm::DiscoverableQueueingRDMController;
using ola::rdm::UID;

JaRuleWidget::JaRuleWidget(ola::io::SelectServerInterface *ss,
                           AsyncronousLibUsbAdaptor *adaptor,
                           libusb_device *device,
                           const UID &controller_uid)
  : m_widget_impl(new JaRuleWidgetImpl(ss, adaptor, device, controller_uid)),
    m_queueing_controller(m_widget_impl.get(), RDM_QUEUE_SIZE) {
}

bool JaRuleWidget::Init() {
  return m_widget_impl->Init();
}

void JaRuleWidget::SendRDMRequest(RDMRequest *request,
                                  ola::rdm::RDMCallback *on_complete) {
  m_queueing_controller.SendRDMRequest(request, on_complete);
}

void JaRuleWidget::RunFullDiscovery(RDMDiscoveryCallback *callback) {
  m_queueing_controller.RunFullDiscovery(callback);
}

void JaRuleWidget::RunIncrementalDiscovery(RDMDiscoveryCallback *callback) {
  m_queueing_controller.RunIncrementalDiscovery(callback);
}

bool JaRuleWidget::SendDMX(const DmxBuffer &buffer) {
  return m_widget_impl->SendDMX(buffer);
}

void JaRuleWidget::ResetDevice() {
  m_widget_impl->ResetDevice();
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
