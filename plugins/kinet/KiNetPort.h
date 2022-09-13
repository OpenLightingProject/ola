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
 * KiNetPort.h
 * The KiNet plugin for ola
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_KINET_KINETPORT_H_
#define PLUGINS_KINET_KINETPORT_H_

#include <string>
#include "ola/network/IPV4Address.h"
#include "ola/strings/Format.h"
#include "olad/Port.h"
#include "plugins/kinet/KiNetDevice.h"
#include "plugins/kinet/KiNetNode.h"

namespace ola {
namespace plugin {
namespace kinet {

class KiNetOutputPort: public BasicOutputPort {
 public:
  KiNetOutputPort(KiNetDevice *device,
                  const ola::network::IPV4Address &target,
                  KiNetNode *node,
                  unsigned int port_id)
      : BasicOutputPort(device, port_id),
        m_node(node),
        m_target(target) {
  }

  bool WriteDMX(const DmxBuffer &buffer, OLA_UNUSED uint8_t priority) {
    // return m_node->SendDMX(m_target, buffer);

    if ((PortId() <= 0) || (PortId() > KINET_PORTOUT_MAX_PORT_COUNT)) {
      OLA_WARN << "Invalid KiNet port id " << PortId();
      return false;
    }
    return m_node->SendPortOut(m_target, PortId(), buffer);
  }

  std::string Description() const {
    return ola::strings::IntToString(PortId());
//    return "Power Supply: " + m_target.ToString();
  }

 private:
  KiNetNode *m_node;
  const ola::network::IPV4Address m_target;
};
}  // namespace kinet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_KINET_KINETPORT_H_
