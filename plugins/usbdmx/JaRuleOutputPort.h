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

#include "libs/usb/JaRulePortHandle.h"
#include "libs/usb/JaRuleWidget.h"

namespace ola {

class Device;

namespace plugin {
namespace usbdmx {

/**
 * @brief A thin wrapper around a JaRulePortHandle so that it can operate as an
 * OLA Port.
 */
class JaRuleOutputPort: public BasicOutputPort {
 public:
  /**
   * @brief Create a new JaRuleOutputPort.
   * @param parent The parent device for this port.
   * @param index The port index, starting from 0
   * @param widget The widget to use.
   */
  JaRuleOutputPort(Device *parent,
                   unsigned int index,
                   ola::usb::JaRuleWidget *widget);

  /**
   * @brief Destructor
   */
  ~JaRuleOutputPort();

  /**
   * @brief Initialize the port.
   * @returns true if ok, false if initialization failed.
   */
  bool Init();

  bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

  std::string Description() const;

  void SendRDMRequest(ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *callback);
  void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
  void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);

  bool PreSetUniverse(Universe *old_universe, Universe *new_universe);
  void PostSetUniverse(Universe *old_universe, Universe *new_universe);

 private:
  const unsigned int m_port_index;
  ola::usb::JaRuleWidget *m_widget;  // not owned
  ola::usb::JaRulePortHandle *m_port_handle;  // not owned

  DISALLOW_COPY_AND_ASSIGN(JaRuleOutputPort);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_JARULEOUTPUTPORT_H_
