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
 * ShowNetDevice.cpp
 * ShowNet device
 * Copyright (C) 2005-2007 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ShowNetDevice.h"
#include "ShowNetPort.h"

#include <llad/logger.h>
#include <llad/Preferences.h>
#include <llad/plugin.h>
#include <llad/universe.h>

#if HAVE_CONFIG_H
#  include <config.h>
#endif


/*
 * Handle dmx from the network, called from libshownet
 *
 * @param n    the shownet_node
 * @param uni  the universe this data is for
 * @param len  the length of the received data
 * @param data  pointer the the dmx data
 * @param d    pointer to our ShowNetDevice
 *
 */
int dmx_handler(shownet_node n, uint8_t uid, int len, uint8_t *data, void *d) {

  ShowNetDevice *dev = (ShowNetDevice *) d;
  ShowNetPort *prt = NULL;
  Universe *uni = NULL;

  if (uid > SHOWNET_MAX_UNIVERSES)
    return 0;

  prt = (ShowNetPort*) dev->GetPort(uid);
  uni = prt->get_universe();

  if ( prt->can_read() && uni != NULL && prt->get_id()%8 == uid) {
    prt->update_buffer(data,len);
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
ShowNetDevice::ShowNetDevice(Plugin *owner, const string &name, Preferences *prefs):
  Device(owner, name),
  m_preferences(prefs),
  m_node(NULL),
  m_enabled(false) {}


/*
 *
 */
ShowNetDevice::~ShowNetDevice() {
  if (m_enabled)
    Stop();
}


/*
 * Start this device
 *
 */
bool ShowNetDevice::Start() {
  ShowNetPort *port = NULL;

  /* set up ports */
  for (int i=0; i < 2*PORTS_PER_DEVICE; i++) {
    port = new ShowNetPort(this, i);

    if (port != NULL)
      this->AddPort(port);
  }

  // create new shownet node, and set config values
  if (m_preferences->GetValue("ip") == "")
    m_node = shownet_new(NULL, get_owner()->debug_on());
  else {
    m_node = shownet_new(m_preferences->GetValue("ip").c_str(), get_owner()->debug_on());
  }

  if (!m_node) {
    Logger::instance()->log(Logger::WARN, "ShowNetPlugin: shownet_new failed: %s", shownet_strerror());
    return -1;
  }

  // setup node
  if (shownet_set_name(m_node, m_preferences->GetValue("name").c_str()) ) {
    Logger::instance()->log(Logger::WARN, "ShowNetPlugin: shownet_set_name failed: %s", shownet_strerror());
    goto e_shownet_start;
  }

  // we want to be notified when the node config changes
  if (shownet_set_dmx_handler(m_node, ::dmx_handler, (void*) this) ) {
    Logger::instance()->log(Logger::WARN, "ShowNetPlugin: shownet_set_dmx_handler failed: %s", shownet_strerror());
    goto e_shownet_start;
  }

  if (shownet_start(m_node) ) {
    Logger::instance()->log(Logger::WARN, "ShowNetPlugin: shownet_start failed: %s", shownet_strerror());
    goto e_shownet_start;
  }

  m_enabled = true;
  return 0;

e_shownet_start:
  if (shownet_destroy(m_node))
    Logger::instance()->log(Logger::WARN, "ShowNetPlugin: shownet_destory failed: %s", shownet_strerror());
  return -1;
}


/*
 * stop this device
 *
 */
bool ShowNetDevice::Stop() {
  Port *prt = NULL;

  if (!m_enabled)
    return 0;

  for (int i=0; i < port_count(); i++) {
    prt = GetPort(i);
    if (prt != NULL)
      delete prt;
  }

  if (shownet_stop(m_node)) {
    Logger::instance()->log(Logger::WARN, "ShowNetPlugin: shownet_stop failed: %s", shownet_strerror());
    return -1;
  }

  if (shownet_destroy(m_node)) {
    Logger::instance()->log(Logger::WARN, "ShowNetPlugin: shownet_destroy failed: %s", shownet_strerror());
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
shownet_node ShowNetDevice::get_node() const {
  return m_node;
}

/*
 * return the sd of this device
 *
 */
int ShowNetDevice::get_sd() const {
  int ret = shownet_get_sd(m_node);

  if (ret < 0) {
    Logger::instance()->log(Logger::WARN, "ShowNetPlugin: shownet_get_sd failed: %s", shownet_strerror());
    return -1;
  }
  return ret;
}

/*
 * Called when there is activity on our descriptors
 *
 * @param  data  user data (pointer to shownet_device_priv
 */
int ShowNetDevice::action() {
  if (shownet_read(m_node, 0) ) {
    Logger::instance()->log(Logger::WARN, "ShowNetPlugin: shownet_read failed: %s", shownet_strerror());
    return -1;
  }
  return 0;
}


// call this when something changes
// where to store data to ?
// I'm thinking a config file in /etc/llad/llad.conf
int ShowNetDevice::SaveConfig() const {


  return 0;
}



/*
 * we can pass plugin specific messages here
 * make sure the user app knows the format though...
 *
 */
int ShowNetDevice::configure(void *req, int len) {
  // handle short/ long name & subnet and port addresses

  req = 0;
  len = 0;

  return 0;
}
