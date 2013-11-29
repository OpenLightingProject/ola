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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * KiNetPort.h
 * The KiNet plugin for ola
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_KINET_KINETPORT_H_
#define PLUGINS_KINET_KINETPORT_H_

#include <string>
#include "ola/network/IPV4Address.h"
#include "olad/Port.h"
#include "plugins/kinet/KiNetDevice.h"
#include "plugins/kinet/KiNetNode.h"

namespace ola {
namespace plugin {
namespace kinet {

using ola::network::IPV4Address;

class KiNetOutputPort: public BasicOutputPort {
 public:
    KiNetOutputPort(KiNetDevice *device,
                    const IPV4Address &target,
                    KiNetNode *node,
                    unsigned int port_id)
        : BasicOutputPort(device, port_id),
          m_node(node),
          m_target(target) {
    }

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
      return m_node->SendDMX(m_target, buffer);
      (void) priority;
    }

    string Description() const {
      return "Power Supply: " + m_target.ToString();
    }

 private:
    KiNetNode *m_node;
    const IPV4Address m_target;
};
}  // namespace kinet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_KINET_KINETPORT_H_
