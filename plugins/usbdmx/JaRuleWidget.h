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
 * JaRuleWidget.h
 * A Ja Rule widget.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef PLUGINS_USBDMX_JARULEWIDGET_H_
#define PLUGINS_USBDMX_JARULEWIDGET_H_

#include <libusb.h>

#include <ola/io/SelectServerInterface.h>
#include <ola/rdm/QueueingRDMController.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/UID.h>

#include <memory>

#include "plugins/usbdmx/LibUsbAdaptor.h"
#include "plugins/usbdmx/Widget.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Represents a Ja Rule hardware widget.
 */
class JaRuleWidget : public Widget,
                     public ola::rdm::DiscoverableRDMControllerInterface {
 public:
  /**
   * @brief Create a new Ja Rule widget.
   * @param ss The SelectServer to run the RDM callbacks on.
   * @param adaptor The LibUsbAdaptor to use.
   * @param device the libusb_device for the Ja Rule widget.
   * @param controller_uid The UID of the controller. This is used for DUB &
   *   Mute / Unmute messages.
   *
   * TODO(simon): Can we instead read the UID from the hardware device itself?
   */
  JaRuleWidget(ola::io::SelectServerInterface *ss,
               AsyncronousLibUsbAdaptor *adaptor,
               libusb_device *device,
               const ola::rdm::UID &controller_uid);

  bool Init();

  void SendRDMRequest(ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *on_complete);
  void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
  void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);

  bool SendDMX(const DmxBuffer &buffer);

  /**
   * @brief Send a reset message to the hardware widget.
   */
  void ResetDevice();

 private:
  std::auto_ptr<class JaRuleWidgetImpl> m_widget_impl;
  ola::rdm::DiscoverableQueueingRDMController m_queueing_controller;

  static const unsigned int RDM_QUEUE_SIZE = 50;

  DISALLOW_COPY_AND_ASSIGN(JaRuleWidget);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_JARULEWIDGET_H_
