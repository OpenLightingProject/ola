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
 * EspNetDevice.cpp
 * Esp-Net device
 * Copyright (C) 2005-2006  Simon Newton
 *
 *
 */

#include <lla/Logging.h>
#include <llad/Plugin.h>
#include <llad/PluginAdaptor.h>
#include <llad/Preferences.h>

#include "EspNetDevice.h"
#include "EspNetNode.h"
#include "EspNetPluginCommon.h"
#include "EspNetPort.h"

namespace lla {
namespace espnet {

const std::string EspNetDevice::IP_KEY = "ip";

/*
 * Handle dmx from the network, called from libespnet
 *
 * @param n    the espnet_node
 * @param uni  the universe this data is for
 * @param len  the length of the received data
 * @param data  pointer the the dmx data
 * @param d    pointer to our EspNetDevice
 *
int dmx_handler(espnet_node n, uint8_t uid, int len, uint8_t *data, void *d) {
  EspNetDevice *device = (EspNetDevice *) d;

  vector<lla::AbstractPort*> ports;
  vector<lla::AbstractPort*>::iterator iter;

  ports = device->Ports();
  for (iter = ports.begin(); iter != ports.end(); ++iter) {
    Universe *universe = (*iter)->GetUniverse();

    if ((*iter)->CanRead() && universe && universe->UniverseId() == uid) {
      ((EspNetPort*) (*iter))->UpdateBuffer(data, len);
    }
  }
  return 0;
}
 */


/*
 * Get notification of remote programming
int program_handler(espnet_node n, void *d) {
  EspNetDevice *dev = (EspNetDevice *) d;

  dev->SaveConfig();
  return 0;
}
 */


/*
 * Create a new device
 * Should prob pass the ip to bind to
 */
EspNetDevice::EspNetDevice(Plugin *owner,
                           const string &name,
                           Preferences *prefs,
                           const PluginAdaptor *plugin_adaptor):
  Device(owner, name),
  m_preferences(prefs),
  m_plugin_adaptor(plugin_adaptor),
  m_node(NULL),
  m_enabled(false) {
}


/*
 * Start this device
 */
bool EspNetDevice::Start() {
  if (m_enabled)
    return false;

  EspNetPort *port = NULL;

  for (unsigned int i = 0; i < 2 * PORTS_PER_DEVICE; i++) {
    port = new EspNetPort(this, i);
    this->AddPort(port);
  }

  m_node = new EspNetNode(m_preferences->GetValue(IP_KEY));
  m_node->SetName(m_preferences->GetValue("name"));
  m_node->SetType(ESPNET_NODE_TYPE_IO);

  if (!m_node->Start()) {
    delete m_node;
    m_node = NULL;
    DeleteAllPorts();
    return false;
  }

  m_plugin_adaptor->AddSocket(m_node->GetSocket());
  m_enabled = true;
  return true;
}


/*
 * Stop this device
 */
bool EspNetDevice::Stop() {
  if (!m_enabled)
    return false;

  m_plugin_adaptor->RemoveSocket(m_node->GetSocket());
  DeleteAllPorts();
  m_node->Stop();
  delete m_node;
  m_node = NULL;
  m_enabled = false;
  return true;

}


} //espnet
} //lla
