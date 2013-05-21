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
 * KiNetDevice.cpp
 * A KiNet Device.
 * Copyright (C) 2013 Simon Newton
 */

#include <memory>
#include <string>
#include <vector>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/NetworkUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Port.h"
#include "plugins/kinet/KiNetDevice.h"
#include "plugins/kinet/KiNetPort.h"

namespace ola {
namespace plugin {
namespace kinet {

using ola::network::IPV4Address;
using std::auto_ptr;
using std::vector;

/*
 * Create a new KiNet Device
 */
KiNetDevice::KiNetDevice(
    AbstractPlugin *owner,
    const std::vector<ola::network::IPV4Address> &power_supplies,
    PluginAdaptor *plugin_adaptor)
    : Device(owner, "KiNet Device"),
      m_power_supplies(power_supplies),
      m_node(NULL),
      m_plugin_adaptor(plugin_adaptor) {
}


/*
 * Start this device
 * @return true on success, false on failure
 */
bool KiNetDevice::StartHook() {
  m_node = new KiNetNode(m_plugin_adaptor);

  if (!m_node->Start()) {
    delete m_node;
    m_node = NULL;
    return false;
  }

  vector<IPV4Address>::const_iterator iter = m_power_supplies.begin();
  unsigned int port_id = 0;
  for (; iter != m_power_supplies.end(); ++iter) {
    AddPort(new KiNetOutputPort(this, *iter, m_node, port_id++));
  }
  return true;
}


/**
 * Stop this device. This is called before the ports are deleted
 */
void KiNetDevice::PrePortStop() {
  m_node->Stop();
}


/*
 * Stop this device
 */
void KiNetDevice::PostPortStop() {
  delete m_node;
  m_node = NULL;
}
}  // namespace kinet
}  // namespace plugin
}  // namespace ola
