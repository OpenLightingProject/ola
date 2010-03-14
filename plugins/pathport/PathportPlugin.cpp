/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * PathportPlugin.cpp
 * The Pathport plugin for ola
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <limits.h>
#include <string>
#include "ola/StringUtils.h"
#include "ola/Logging.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/pathport/PathportDevice.h"
#include "plugins/pathport/PathportPlugin.h"


namespace ola {
namespace plugin {
namespace pathport {

const char PathportPlugin::PATHPORT_DEVICE_NAME[] = "Pathport Device";
const char PathportPlugin::PLUGIN_NAME[] = "Pathport Plugin";
const char PathportPlugin::PLUGIN_PREFIX[] = "pathport";
const char PathportPlugin::DEFAULT_DSCP_VALUE[] = "0";


/*
 * Start the plugin
 * For now we just have one device.
 */
bool PathportPlugin::StartHook() {
  m_device = new PathportDevice(this,
                                PATHPORT_DEVICE_NAME,
                                m_preferences,
                                m_plugin_adaptor);

  if (!m_device)
    return false;

  if (!m_device->Start()) {
    delete m_device;
    return false;
  }
  m_plugin_adaptor->RegisterDevice(m_device);
  return true;
}


/*
 * Stop the plugin
 * @return true on sucess, false on failure
 */
bool PathportPlugin::StopHook() {
  m_plugin_adaptor->UnregisterDevice(m_device);
  m_device->Stop();
  delete m_device;
  return true;
}


/*
 * Return the description for this plugin
 */
string PathportPlugin::Description() const {
  return
"Pathport Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates a single device with 5 input and 5 output ports.\n"
"\n"
"The universe the port is patched to corresponds with the DMX channels used \n"
"in the pathport protocol. For example universe 0 is xDMX channels 0 - 511, \n"
"universe 1 is xDMX channels 512 - 1023.\n"
"\n"
"--- Config file : ola-pathport.conf ---\n"
"\n"
"dscp = <int>\n"
"Set the DSCP value for the packets. Range is 0-63"
"ip = a.b.c.d\n"
"The ip address to bind to. If not specified it will use the first \n"
"non-loopback ip.\n"
"\n"
"name = ola-Pathport\n"
"The name of the node.\n"
"\n"
"node-id = <int>\n"
"The pathport id of the node\n"
"\n";
}


/*
 * Load the plugin prefs and default to sensible values
 */
bool PathportPlugin::SetDefaultPreferences() {
  bool save = false;

  if (!m_preferences)
    return false;

  save |= m_preferences->SetDefaultValue(PathportDevice::K_DSCP_KEY,
                                         IntValidator(0, 63),
                                         DEFAULT_DSCP_VALUE);
  save |= m_preferences->SetDefaultValue(PathportDevice::K_NODE_IP_KEY,
                                         IPv4Validator(), "");
  save |= m_preferences->SetDefaultValue(PathportDevice::K_NODE_NAME_KEY,
                                         StringValidator(),
                                         PathportDevice::K_DEFAULT_NODE_NAME);

  // generate a new node id in case we need it
  srand((unsigned)time(0) * getpid());
  uint32_t product_id = OLA_MANUFACTURER_CODE << 24;
  product_id |= (rand() / (RAND_MAX / 0x100) << 16);
  product_id |= (rand() / (RAND_MAX / 0x100) << 8);
  product_id |= rand() / (RAND_MAX / 0x100);

  save |= m_preferences->SetDefaultValue(PathportDevice::K_NODE_ID_KEY,
                                         IntValidator(0, UINT_MAX),
                                         IntToString(product_id));

  if (save)
    m_preferences->Save();

  if (m_preferences->GetValue(PathportDevice::K_NODE_NAME_KEY).empty() ||
      m_preferences->GetValue(PathportDevice::K_NODE_ID_KEY).empty())
    return false;

  return true;
}
}  // pathport
}  // plugin
}  // ola
