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
 * NanoleafPort.h
 * The Nanoleaf plugin for ola
 * Copyright (C) 2017 Peter Newman
 */

#ifndef PLUGINS_NANOLEAF_NANOLEAFPORT_H_
#define PLUGINS_NANOLEAF_NANOLEAFPORT_H_

#include <string>
#include "ola/network/SocketAddress.h"
#include "olad/Port.h"
#include "plugins/nanoleaf/NanoleafDevice.h"
#include "plugins/nanoleaf/NanoleafNode.h"

namespace ola {
namespace plugin {
namespace nanoleaf {

class NanoleafOutputPort: public BasicOutputPort {
 public:
  NanoleafOutputPort(NanoleafDevice *device,
                     const ola::network::IPV4SocketAddress &target,
                     NanoleafNode *node,
                     unsigned int port_id)
      : BasicOutputPort(device, port_id),
        m_node(node),
        m_target(target) {
  }

  bool WriteDMX(const DmxBuffer &buffer, OLA_UNUSED uint8_t priority) {
    return m_node->SendDMX(m_target, buffer);
  }

  std::string Description() const {
    return "Controller: " + m_target.Host().ToString();
  }

 private:
  NanoleafNode *m_node;
  const ola::network::IPV4SocketAddress m_target;
};
}  // namespace nanoleaf
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_NANOLEAF_NANOLEAFPORT_H_
