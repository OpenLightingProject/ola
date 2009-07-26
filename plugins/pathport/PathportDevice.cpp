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
 * pathportdevice.cpp
 * Pathport device
 * Copyright (C) 2005  Simon Newton
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ola/Logging.h>
#include <olad/Preferences.h>
#include <olad/universe.h>

#include "PathportDevice.h"
#include "PathportPort.h"
#include "PathportCommon.h"

#if HAVE_CONFIG_H
#  include <config.h>
#endif

namespace ola {
namespace plugin {

/*
 * Handle dmx from the network, called from libpathport
 *
 * @param n    the pathport_node
 * @param uni  the universe this data is for
 * @param len  the length of the received data
 * @param data  pointer the the dmx data
 * @param d    pointer to our PathportDevice
 *
 */
int dmx_handler(pathport_node n, unsigned int uid, unsigned int len,
                const uint8_t *data, void *d) {

  PathportDevice *dev = (PathportDevice *) d;
  PathportPort *prt;
  Universe *uni;

  if ( uid > PATHPORT_MAX_UNIVERSES)
    return 0;

  prt = (PathportPort*) dev->GetPort_from_uni(uid);
  uni = prt->get_universe();

  if ( prt != NULL && prt->can_read() && uni != NULL) {
    prt->update_buffer(data, len);
  }

  n = NULL;
  return 0;
}



/*
 * Create a new device
 *
 * should prob pass the ip to bind to
 *
 */
PathportDevice::PathportDevice(Plugin *owner,
                               const string &name,
                               Preferences *prefs,
                               const PluginAdaptor *plugin_adaptor) {
  Device(owner, name),
  m_preferences(prefs),
  m_plugin_adaptor(plugin_adaptor),
  m_node(NULL),
  m_enabled(false) {
}


/*
 *
 */
PathportDevice::~PathportDevice() {
  if (m_enabled)
    Stop();
}


/*
 * Start this device
 *
 */
bool PathportDevice::Start() {
  PathportPort *port = NULL;

  /* set up ports */
  for (int i=0; i < PORTS_PER_DEVICE; i++) {
    port = new PathportPort(this, i);

    if (port)
      this->AddPort(port);
  }

  bool debug = Owner()->DebugOn();
  // create new pathport node, and set config values
    if (m_preferences->GetValue("ip") == "")
    m_node = pathport_new(NULL, debug);
  else {
    m_node = pathport_new(m_preferences->GetValue("ip").c_str(), debug);
  }

  if (!m_node) {
    OLA_WARN << "pathport_new failed: " << pathport_strerror();
    return false;
  }

  // setup node
  if (pathport_set_name(m_node, m_preferences->GetValue("name").c_str()) ) {
    OLA_WARN << "pathport_set_name failed: " << pathport_strerror();
    goto e_pathport_start;
  }

  // setup node
  if (pathport_set_type(m_node, PATHPORT_MANUF_ZP_TECH, PATHPORT_CLASS_NODE, PATHPORT_CLASS_NODE_PATHPORT) ) {
    OLA_WARN << "pathport_set_type failed: " << pathport_strerror();
    goto e_pathport_start;
  }

  // we want to be notified when the node config changes
  if (pathport_set_dmx_handler(m_node, ::dmx_handler, (void*) this) ) {
    OLA_WARN << "pathport_set_dmx_handler failed: " << pathport_strerror();
    goto e_pathport_start;
  }

  if (pathport_start(m_node) ) {
    OLA_WARN << "pathport_start failed: " << pathport_strerror();
    goto e_pathport_start;
  }

  m_enabled = true;
  return true;

e_pathport_start:
  if (pathport_destroy(m_node))
    OLA_WARN << "pathport_destory failed: " << pathport_strerror();
  return -1;
}


/*
 * stop this device
 *
 */
bool PathportDevice::Stop() {
  Port *prt = NULL;

  if (!m_enabled)
    return 0;

  for (int i=0; i < port_count(); i++) {
    prt = GetPort(i);
    if (prt != NULL)
      delete prt;
  }

  if (pathport_stop(m_node)) {
    OLA_WARN << "pathport_stop failed: " << pathport_strerror();
    return -1;
  }

  if (pathport_destroy(m_node)) {
    OLA_WARN << "pathport_destroy failed: " << pathport_strerror();
    return -1;
  }

  m_enabled = false;

  return 0;
}


/*
 * return the Art-Net node associated with this device
 *
 *
 */
pathport_node PathportDevice::get_node() const {
  return m_node;
}

/*
 * return the sd of this device
 *
 */
int PathportDevice::get_sd(unsigned int i) const {
  int ret = pathport_get_sd(m_node, i);

  if (ret < 0) {
    OLA_WARN << "pathport_get_sd failed: " << pathport_strerror();
    return -1;
  }
  return ret;
}

/*
 * Called when there is activity on our descriptors
 *
 * @param  data  user data (pointer to pathport_device_priv
 */
int PathportDevice::action() {
  if (pathport_read(m_node, 0) ) {
    OLA_WARN << "pathport_read failed: " << pathport_strerror();
    return -1;
  }
  return 0;
}


/*
 * Add a port to our hash map
 */
int PathportDevice::port_map(Universe *uni, PathportPort *prt) {
    m_portmap[uni->get_uid()] = prt;
    return 0;
}

PathportPort *PathportDevice::GetPort_from_uni(int uni) {
    return m_portmap[uni];
}

} //plugin
} //ola

