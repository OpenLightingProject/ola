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
#include <set>
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
using std::set;
using std::string;
using std::vector;

const char KiNetDevice::KINET_DEVICE_NAME[] = "KiNET";
const char KiNetDevice::DMXOUT_MODE[] = "dmxout";
const char KiNetDevice::PORTOUT_MODE[] = "portout";

/*
 * Create a new KiNet Device
 */
KiNetDevice::KiNetDevice(
    AbstractPlugin *owner,
    const ola::network::IPV4Address &power_supply,
    PluginAdaptor *plugin_adaptor,
    KiNetNode *node,
    Preferences *preferences)
    // TODO(Peter): Override this name properly
    : Device(owner, KINET_DEVICE_NAME),
      m_power_supply(power_supply),
      m_plugin_adaptor(plugin_adaptor),
      m_node(node),
      m_preferences(preferences) {
}


string KiNetDevice::DeviceId() const {
  return m_power_supply.ToString();
}


string KiNetDevice::ModeKey(const ola::network::IPV4Address &power_supply) {
  return power_supply.ToString() + "-mode";
}

string KiNetDevice::ModeKey() const {
  return ModeKey(m_power_supply);
}


// TODO(Peter): Inherit and call this too!
void KiNetDevice::SetDefaults() {
  // Set device options
  set<string> valid_modes;
  valid_modes.insert(DMXOUT_MODE);
  valid_modes.insert(PORTOUT_MODE);
  m_preferences->SetDefaultValue(ModeKey(),
                                 SetValidator<string>(valid_modes),
                                 DMXOUT_MODE);
  m_preferences->Save();
}

const char KiNetPortOutDevice::KINET_PORT_OUT_DEVICE_NAME[] =
    "KiNET Port Out Mode";

/*
 * Create a new KiNET Port Out Device
 */
KiNetPortOutDevice::KiNetPortOutDevice(
    AbstractPlugin *owner,
    const ola::network::IPV4Address &power_supply,
    PluginAdaptor *plugin_adaptor,
    KiNetNode *node,
    Preferences *preferences)
    : KiNetDevice(owner,
                  power_supply,
                  plugin_adaptor,
                  node,
                  preferences) {
}


/*
 * Start this device
 * @return true on success, false on failure
 */
bool KiNetPortOutDevice::StartHook() {
  ostringstream str;
  str << KINET_PORT_OUT_DEVICE_NAME << " [Power Supply "
      << m_power_supply.ToString() << "]";
  SetName(str.str());

  SetDefaults();

  uint8_t port_count;
  if (!StringToInt(m_preferences->GetValue(PortCountKey()), &port_count)) {
    port_count = KINET_PORTOUT_MAX_PORT_COUNT;
  } else if ((port_count < 1) || (port_count > KINET_PORTOUT_MAX_PORT_COUNT)) {
    OLA_WARN << "Invalid port count value for " << PortCountKey();
  }

  // TODO(Peter): Split this off!
  AddPort(new KiNetDmxOutOutputPort(this, m_power_supply, m_node));
  for (uint8_t i = 1; i <= port_count; i++) {
    AddPort(new KiNetPortOutOutputPort(this, m_power_supply, m_node, i));
  }
  return true;
}


string KiNetPortOutDevice::PortCountKey() const {
  return m_power_supply.ToString() + "-ports";
}

void KiNetPortOutDevice::SetDefaults() {
  // Set device options
  m_preferences->SetDefaultValue(
      PortCountKey(),
      UIntValidator(1, KINET_PORTOUT_MAX_PORT_COUNT),
      KINET_PORTOUT_MAX_PORT_COUNT);
  m_preferences->Save();
}

const char KiNetDmxOutDevice::KINET_DMX_OUT_DEVICE_NAME[] =
    "KiNET DMX Out Mode";

/*
 * Create a new KiNET DMX Out Device
 */
KiNetDmxOutDevice::KiNetDmxOutDevice(
    AbstractPlugin *owner,
    const ola::network::IPV4Address &power_supply,
    PluginAdaptor *plugin_adaptor,
    KiNetNode *node,
    Preferences *preferences)
    : KiNetDevice(owner,
                  power_supply,
                  plugin_adaptor,
                  node,
                  preferences) {
}


/*
 * Start this device
 * @return true on success, false on failure
 */
bool KiNetDmxOutDevice::StartHook() {
  ostringstream str;
  str << KINET_DMX_OUT_DEVICE_NAME << " [Power Supply "
      << m_power_supply.ToString() << "]";
  SetName(str.str());

  AddPort(new KiNetDmxOutOutputPort(this, m_power_supply, m_node));
  return true;
}
}  // namespace kinet
}  // namespace plugin
}  // namespace ola
