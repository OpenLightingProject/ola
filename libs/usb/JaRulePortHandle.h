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
 * JaRulePortHandle.h
 * A Ja Rule Port Handle.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef LIBS_USB_JARULEPORTHANDLE_H_
#define LIBS_USB_JARULEPORTHANDLE_H_

#include <stdint.h>

#include <ola/DmxBuffer.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/QueueingRDMController.h>

#include <memory>

#include "libs/usb/JaRuleConstants.h"

namespace ola {
namespace usb {

/**
 * @brief Represents a DMX/RDM port on a Ja Rule device.
 */
class JaRulePortHandle : public ola::rdm::DiscoverableRDMControllerInterface {
 public:
  /**
   * @brief Create a new Ja Rule Port Handle.
   */
  JaRulePortHandle(class JaRuleWidgetPort *parent_port,
                   const ola::rdm::UID &uid,
                   uint8_t physical_port);
  ~JaRulePortHandle();

  void SendRDMRequest(ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *on_complete);
  void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
  void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);

  /**
   * @brief Send DMX data from this widget
   * @param buffer The DmxBuffer containing the data to send.
   * @returns true if the data was sent, false otherwise.
   */
  bool SendDMX(const DmxBuffer &buffer);

  /**
   * @brief Change the mode of the port.
   *
   * @todo I need to think about how to return errors from this since it's
   * async.
   */
  bool SetPortMode(JaRulePortMode new_mode);

 private:
  // Order of destruction is important.
  std::auto_ptr<class JaRulePortHandleImpl> m_impl;
  ola::rdm::DiscoverableQueueingRDMController m_queueing_controller;

  static const unsigned int RDM_QUEUE_SIZE = 50;

  DISALLOW_COPY_AND_ASSIGN(JaRulePortHandle);
};
}  // namespace usb
}  // namespace ola
#endif  // LIBS_USB_JARULEPORTHANDLE_H_
