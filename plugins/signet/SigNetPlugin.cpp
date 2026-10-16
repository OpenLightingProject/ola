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
 * SigNetPlugin.cpp
 * The SigNet plugin for ola. This creates a single SigNet device.
 * Copyright (C) 2026 Peter Newman
 */

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ola/StringUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/signet/SigNetDevice.h"
#include "plugins/signet/SigNetPlugin.h"
#include "plugins/signet/SigNetPluginDescription.h"

namespace ola {
namespace plugin {
namespace signet {

using ola::network::IPV4SocketAddress;
using std::set;
using std::string;
using std::vector;

const char SigNetPlugin::INPUT_PORT_COUNT_KEY[] = "input_ports";
const char SigNetPlugin::OUTPUT_PORT_COUNT_KEY[] = "output_ports";
const char SigNetPlugin::PLUGIN_NAME[] = "SigNet";
const char SigNetPlugin::PLUGIN_PREFIX[] = "signet";

/*
 * Start the plugin.
 */
bool SigNetPlugin::StartHook() {
  SigNetDevice::SigNetDeviceOptions options;

  if (!StringToInt(m_preferences->GetValue(INPUT_PORT_COUNT_KEY),
                   &options.input_ports)) {
    OLA_WARN << "Invalid value for input_ports";
  }

  if (!StringToInt(m_preferences->GetValue(OUTPUT_PORT_COUNT_KEY),
                   &options.output_ports)) {
    OLA_WARN << "Invalid value for output_ports";
  }

  // Finally create the new SigNetDevice, start it and register the device.
  std::auto_ptr<SigNetDevice> device(
    new SigNetDevice(this, m_plugin_adaptor, options));
  if (!device->Start()) {
    OLA_FATAL << "SigNet device didn't start!";
    return false;
  }
  m_device = device.release();
  m_plugin_adaptor->RegisterDevice(m_device);
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool SigNetPlugin::StopHook() {
  if (m_device) {
    m_plugin_adaptor->UnregisterDevice(m_device);
    bool ret = m_device->Stop();
    delete m_device;
    return ret;
  }
  return true;
}


string SigNetPlugin::Description() const {
  return plugin_description;
}


/**
 * Set the default preferences for the SigNet plugin.
 */
bool SigNetPlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }

  bool save = false;

  save |= m_preferences->SetDefaultValue(INPUT_PORT_COUNT_KEY,
                                         UIntValidator(0, 32),
                                         DEFAULT_PORT_COUNT);

  save |= m_preferences->SetDefaultValue(OUTPUT_PORT_COUNT_KEY,
                                         UIntValidator(0, 32),
                                         DEFAULT_PORT_COUNT);

  if (save) {
    m_preferences->Save();
  }

  return true;
}
}  // namespace signet
}  // namespace plugin
}  // namespace ola
