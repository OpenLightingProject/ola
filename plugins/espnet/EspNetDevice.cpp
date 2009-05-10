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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <lla/Logging.h>
#include <llad/Plugin.h>
#include <llad/Universe.h>

#include "EspNetDevice.h"
#include "EspNetPort.h"

#if HAVE_CONFIG_H
#  include <config.h>
#endif

namespace lla {
namespace plugin {

/*
 * Handle dmx from the network, called from libespnet
 *
 * @param n    the espnet_node
 * @param uni  the universe this data is for
 * @param len  the length of the received data
 * @param data  pointer the the dmx data
 * @param d    pointer to our EspNetDevice
 *
 */
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


/*
 * Get notification of remote programming
 */
int program_handler(espnet_node n, void *d) {
  EspNetDevice *dev = (EspNetDevice *) d;

  dev->SaveConfig();
  return 0;
}


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
  m_socket(NULL),
  m_enabled(false) {
}


/*
 * Cleanup
 */
EspNetDevice::~EspNetDevice() {
  if (m_enabled)
    Stop();
}


/*
 * Start this device
 *
 */
bool EspNetDevice::Start() {
  /* set up ports */
  for(int i=0; i < 2 * PORTS_PER_DEVICE; i++) {
    EspNetPort *port = new EspNetPort(this, i);
    this->AddPort(port);
  }

  // create new espnet node, and set config values
  int sd;

  if (m_preferences->GetValue("ip").empty())
    m_node = espnet_new(NULL, Owner()->DebugOn());
  else {
    m_node = espnet_new(m_preferences->GetValue("ip").data(),
                        Owner()->DebugOn());
  }

  if (!m_node) {
    LLA_WARN << "espnet_new failed: " << espnet_strerror();
    return false;
  }

  // setup node
  if (espnet_set_name(m_node, m_preferences->GetValue("name").data())) {
    LLA_WARN << "espnet_set_name failed: " << espnet_strerror();
    goto e_espnet_start;
  }

  if (espnet_set_type(m_node, ESPNET_NODE_TYPE_IO)) {
    LLA_WARN << "espnet_set_type failed: " << espnet_strerror();
    goto e_espnet_start;
  }

  // we want to be notified when the node config changes
  if (espnet_set_dmx_handler(m_node, lla::plugin::dmx_handler, (void*) this)) {
    LLA_WARN << "espnet_set_dmx_handler failed: " << espnet_strerror();
    goto e_espnet_start;
  }

  if (espnet_start(m_node) ) {
    LLA_WARN << "espnet_start failed: " << espnet_strerror() ;
    goto e_espnet_start ;
  }

  sd = espnet_get_sd(m_node);
  m_socket = new ConnectedSocket(sd, sd);
  m_enabled = true;
  return true;

e_espnet_start:
  if (espnet_destroy(m_node))
    LLA_WARN << "espnet_destroy failed: " << espnet_strerror();
  return false;
}


/*
 * stop this device
 */
bool EspNetDevice::Stop() {
  if (!m_enabled)
    return true;

  DeleteAllPorts();

  if(espnet_stop(m_node)) {
    LLA_WARN << "espnet_stop failed: " << espnet_strerror();
    return false;
  }

  if(espnet_destroy(m_node)) {
    LLA_WARN << "espnet_destroy failed: " << espnet_strerror();
    return false;
  }

  delete m_socket;
  m_socket = NULL;
  m_enabled = false;
  return true;
}


/*
 * return the Art-Net node associated with this device
 *
 *
 */
espnet_node EspNetDevice::EspnetNode() const {
  return m_node;
}


/*
 * Called when there is activity on our descriptors
 *
 * @param  data  user data (pointer to espnet_device_priv
 */
int EspNetDevice::SocketReady(ConnectedSocket *socket) {
  if (espnet_read(m_node, 0)) {
    LLA_WARN << "espnet_read failed: " << espnet_strerror();
    return -1;
  }
  socket = 0;
  return 0;
}

} //plugin
} //lla
