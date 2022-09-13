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
 * KiNetDevice.cpp
 * A KiNet Device.
 * Copyright (C) 2013 Simon Newton
 */

#include <memory>
#include <sstream>
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
#include "olad/Preferences.h"
#include "plugins/kinet/KiNetDevice.h"
#include "plugins/kinet/KiNetPort.h"

namespace ola {
namespace plugin {
namespace kinet {

using ola::network::IPV4Address;
using std::auto_ptr;
using std::ostringstream;
using std::string;
using std::vector;

const char KiNetDevice::KINET_DEVICE_NAME[] = "KiNet";

/*
 * Create a new KiNet Device
 */
KiNetDevice::KiNetDevice(
    AbstractPlugin *owner,
    const ola::network::IPV4Address &power_supply,
    PluginAdaptor *plugin_adaptor,
    KiNetNode *node,
    Preferences *preferences)
    : Device(owner, KINET_DEVICE_NAME),
      m_power_supply(power_supply),
      m_plugin_adaptor(plugin_adaptor),
      m_node(node),
      m_preferences(preferences) {
}


/*
 * Start this device
 * @return true on success, false on failure
 */
bool KiNetDevice::StartHook() {
  ostringstream str;
  str << KINET_DEVICE_NAME << " [Power Supply " << m_power_supply.ToString()
      << "]";
  SetName(str.str());

  for (uint8_t i = 1; i <= KINET_PORTOUT_MAX_PORT_COUNT; i++) {
    AddPort(new KiNetOutputPort(this, m_power_supply, m_node, i));
  }
  return true;
}


string KiNetDevice::DeviceId() const {
  return m_power_supply.ToString();
}


//string NanoleafDevice::IPPortKey() const {
//  return m_controller.ToString() + "-port";
//}
}  // namespace kinet
}  // namespace plugin
}  // namespace ola
