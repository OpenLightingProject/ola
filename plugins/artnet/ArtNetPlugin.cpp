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
 * ArtNetPlugin.cpp
 * The Art-Net plugin for ola
 * Copyright (C) 2005 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "ola/Logging.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/artnet/ArtNetPlugin.h"
#include "plugins/artnet/ArtNetPluginDescription.h"
#include "plugins/artnet/ArtNetDevice.h"


namespace ola {
namespace plugin {
namespace artnet {

using std::string;

const char ArtNetPlugin::ARTNET_LONG_NAME[] = "OLA - ArtNet node";
const char ArtNetPlugin::ARTNET_SHORT_NAME[] = "OLA - ArtNet node";
const char ArtNetPlugin::PLUGIN_NAME[] = "ArtNet";
const char ArtNetPlugin::PLUGIN_PREFIX[] = "artnet";

bool ArtNetPlugin::StartHook() {
  m_device = new ArtNetDevice(this,
                              m_preferences,
                              m_plugin_adaptor);

  if (!m_device->Start()) {
    delete m_device;
    return false;
  }
  // Register device will restore the port settings. To avoid a flurry of
  // ArtPoll / ArtPollReply messages, we enter config mode here.
  m_device->EnterConfigurationMode();
  m_plugin_adaptor->RegisterDevice(m_device);
  m_device->ExitConfigurationMode();
  return true;
}


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


string ArtNetPlugin::Description() const {
  return plugin_description;
}


bool ArtNetPlugin::SetDefaultPreferences() {
  bool save = false;

  if (!m_preferences) {
    return false;
  }

  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_IP_KEY,
                                         StringValidator(true), "");
  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_SHORT_NAME_KEY,
                                         StringValidator(),
                                         ARTNET_SHORT_NAME);
  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_LONG_NAME_KEY,
                                         StringValidator(),
                                         ARTNET_LONG_NAME);
  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_NET_KEY,
                                         UIntValidator(0, 127),
                                         ArtNetDevice::K_ARTNET_NET);
  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_SUBNET_KEY,
                                         UIntValidator(0, 15),
                                         ArtNetDevice::K_ARTNET_SUBNET);
  save |= m_preferences->SetDefaultValue(
      ArtNetDevice::K_OUTPUT_PORT_KEY,
      UIntValidator(0, 16),
      ArtNetDevice::K_DEFAULT_OUTPUT_PORT_COUNT);
  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_ALWAYS_BROADCAST_KEY,
                                         BoolValidator(),
                                         false);
  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_LIMITED_BROADCAST_KEY,
                                         BoolValidator(),
                                         false);
  save |= m_preferences->SetDefaultValue(ArtNetDevice::K_LOOPBACK_KEY,
                                         BoolValidator(),
                                         false);

  if (save) {
    m_preferences->Save();
  }

  // check if this saved correctly
  // we don't want to use it if null
  if (m_preferences->GetValue(ArtNetDevice::K_SHORT_NAME_KEY).empty() ||
      m_preferences->GetValue(ArtNetDevice::K_LONG_NAME_KEY).empty() ||
      m_preferences->GetValue(ArtNetDevice::K_SUBNET_KEY).empty() ||
      m_preferences->GetValue(ArtNetDevice::K_OUTPUT_PORT_KEY).empty() ||
      m_preferences->GetValue(ArtNetDevice::K_NET_KEY).empty()) {
    return false;
  }

  return true;
}
}  // namespace artnet
}  // namespace plugin
}  // namespace ola
