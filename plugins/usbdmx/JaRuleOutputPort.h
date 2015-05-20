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
 * JaRuleOutputPort.h
 * A JaRule output port that uses a widget.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef PLUGINS_USBDMX_JARULEOUTPUTPORT_H_
#define PLUGINS_USBDMX_JARULEOUTPUTPORT_H_

#include <string>
#include "ola/base/Macro.h"
#include "olad/Port.h"

namespace ola {

class Device;

namespace plugin {
namespace usbdmx {

/**
 * @brief A thin wrapper around a JaRuleWidget so that it can operate as an OLA
 * Port.
 */
class JaRuleOutputPort: public BasicOutputPort {
 public:
  /**
   * @brief Create a new JaRuleOutputPort.
   * @param parent The parent device for this port.
   * @param id The port id.
   * @param widget The widget to use to send DMX frames.
   */
  JaRuleOutputPort(Device *parent,
                   unsigned int id,
                   class JaRuleWidget *widget);

  bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

  std::string Description() const { return ""; }

  void SendRDMRequest(ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *callback);
  void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
  void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);

 private:
  class JaRuleWidget* const m_widget;

  DISALLOW_COPY_AND_ASSIGN(JaRuleOutputPort);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_JARULEOUTPUTPORT_H_
