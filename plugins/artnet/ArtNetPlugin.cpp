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
 * ArtNetPlugin.cpp
 * The ArtNet plugin for ola
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "ola/Logging.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/artnet/ArtNetPlugin.h"
#include "plugins/artnet/ArtNetDevice.h"


namespace ola {
namespace plugin {
namespace artnet {

const char ArtNetPlugin::ARTNET_LONG_NAME[] = "OLA - ArtNet node";
const char ArtNetPlugin::ARTNET_SHORT_NAME[] = "OLA - ArtNet node";
const char ArtNetPlugin::ARTNET_NET[] = "0";
const char ArtNetPlugin::ARTNET_SUBNET[] = "0";
const char ArtNetPlugin::PLUGIN_NAME[] = "ArtNet";
const char ArtNetPlugin::PLUGIN_PREFIX[] = "artnet";

/*
 * Start the plugin, for now we just have one device.
 * TODO: allow multiple devices on different IPs ?
 * @returns true if we started ok, false otherwise
 */
bool ArtNetPlugin::StartHook() {
  m_device = new ArtNetDevice(this,
                              m_preferences,
                              m_plugin_adaptor);

  if (!m_device->Start()) {
    delete m_device;
    return false;
  }
  m_plugin_adaptor->RegisterDevice(m_device);
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool ArtNetPlugin::StopHook() {
  if (m_device) {
    // stop the device
    m_plugin_adaptor->UnregisterDevice(m_device);
    bool ret = m_device->Stop();
    delete m_device;
    return ret;
  }
  return true;
}


/*
 * Return the description for this plugin.
 * @return a string description of the plugin
 */
string ArtNetPlugin::Description() const {
  return
      "ArtNet Plugin\n"
      "----------------------------\n"
      "\n"
      "This plugin creates a single device with four input and four output ports \n"
      "and supports ArtNet, ArtNet 2 and ArtNet 3.\n"
      "\n"
      "ArtNet limits a single device (identified by a unique IP) to four input and\n"
      "four output ports, each bound to a separate ArtNet Port Address (see the \n"
      "ArtNet spec for more details). The ArtNet Port Address is a 16 bits int, \n"
      "defined as follows: \n"
      "\n"
      " Bit 15 | Bits 14 - 8 | Bits 7 - 4 | Bits 3 - 0\n"
      " 0      |   Net       | Sub-Net    | Universe\n"
      "\n"
      "For OLA, the Net and Sub-Net values can be controlled by the config file.\n"
      "The Universe bits are the OLA Universe number modulo 16.\n"
      "\n"
      " ArtNet Net | ArtNet Subnet | OLA Universe | ArtNet Port Address\n"
      " 0          | 0             | 0            | 0\n"
      " 0          | 0             | 1            | 1\n"
      " 0          | 0             | 15           | 15\n"
      " 0          | 0             | 16           | 0\n"
      " 0          | 0             | 17           | 1\n"
      " 0          | 1             | 0            | 16\n"
      " 0          | 1             | 1            | 17\n"
      " 0          | 15            | 0            | 240\n"
      " 0          | 15            | 15           | 255\n"
      " 1          | 0             | 0            | 256\n"
      " 1          | 0             | 1            | 257\n"
      " 1          | 0             | 15           | 271\n"
      " 1          | 1             | 0            | 272\n"
      " 1          | 15            | 0            | 496\n"
      " 1          | 15            | 15           | 511\n"
      "\n"
      "That is Port Address = (Net << 8) + (Subnet << 4) + (Universe % 4)\n"
      "\n"
      "--- Config file : ola-artnet.conf ---\n"
      "\n"
      "always_broadcast = [true|false]\n"
      "Use ArtNet v1 and always broadcast the DMX data. Turn this on if\n"
      "you have devices that don't respond to ArtPoll messages.\n"
      "\n"
      "ip = [a.b.c.d|<interface_name>]\n"
      "The ip address or interface name to bind to. If not specified it will\n"
      "use the first non-loopback interface.\n"
      "\n"
      "long_name = ola - ArtNet node\n"
      "The long name of the node.\n"
      "\n"
      "net = 0\n"
      "The ArtNet Net to use (0-127).\n"
      "\n"
      "short_name = ola - ArtNet node\n"
      "The short name of the node (first 17 chars will be used).\n"
      "\n"
      "subnet = 0\n"
      "The ArtNet subnet to use (0-15).\n"
      "\n"
      "use_limited_broadcast = [true|false]\n"
      "When broadcasting, use the limited broadcast address (255.255.255.255) \n"
      "rather than the subnet directed broadcast address. Some devices which \n"
      "don't follow the ArtNet spec require this.\n"
      "\n"
      "use_loopback = [true|false]\n"
      "Enable use of the loopback device.\n"
      "\n";
}


/*
 * Set default preferences.
 */
bool ArtNetPlugin::SetDefaultPreferences() {
  bool save = false;

  if (!m_preferences)
    return false;

  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_IP_KEY,
                                         StringValidator(true), "");
  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_SHORT_NAME_KEY,
                                         StringValidator(),
                                         ARTNET_SHORT_NAME);
  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_LONG_NAME_KEY,
                                         StringValidator(),
                                         ARTNET_LONG_NAME);
  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_NET_KEY,
                                         IntValidator(0, 127),
                                         ARTNET_NET);
  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_SUBNET_KEY,
                                         IntValidator(0, 15),
                                         ARTNET_SUBNET);
  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_ALWAYS_BROADCAST_KEY,
                                         BoolValidator(),
                                         BoolValidator::DISABLED);
  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_LIMITED_BROADCAST_KEY,
                                         BoolValidator(),
                                         BoolValidator::DISABLED);
  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_LOOPBACK_KEY,
                                         BoolValidator(),
                                         BoolValidator::DISABLED);

  if (save)
    m_preferences->Save();

  // check if this save correctly
  // we don't want to use it if null
  if (m_preferences->GetValue(ArtNetDevice::K_SHORT_NAME_KEY).empty() ||
      m_preferences->GetValue(ArtNetDevice::K_LONG_NAME_KEY).empty() ||
      m_preferences->GetValue(ArtNetDevice::K_SUBNET_KEY).empty() ||
      m_preferences->GetValue(ArtNetDevice::K_NET_KEY).empty())
    return false;

  return true;
}
}  // namespace artnet
}  // namespace plugin
}  // namespace ola
