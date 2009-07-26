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
 * sandnetdevice.cpp
 * SandNet device
 * Copyright (C) 2005  Simon Newton
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SandnetDevice.h"
#include "SandnetPort.h"

#include <ola/Logging.h>
#include <olad/preferences.h>
#include <olad/universe.h>

#if HAVE_CONFIG_H
#  include <config.h>
#endif


/*
 * Handle dmx from the network, called from libsandnet
 *
 * @param n    the sandnet_node
 * @param uni  the universe this data is for
 * @param len  the length of the received data
 * @param data  pointer the the dmx data
 * @param d    pointer to our SandNetDevice
 *
 */
int dmx_handler(sandnet_node n, uint8_t grp, uint8_t uid, int len, uint8_t *data, void *d) {
  SandNetDevice *dev = (SandNetDevice *) d;
  SandNetPort *prt = dev->get_port_from_uni(uid);

  if (prt != NULL) {
    prt->update_buffer(data,len);
  }

  grp = 0;
  n = NULL;
  return 0;
}


/*
 * Create a new device
 *
 * should prob pass the ip to bind to
 *
 */
SandNetDevice::SandNetDevice(Plugin *owner, const string &name, Preferences *prefs) :
  Device(owner, name),
  m_prefs(prefs),
  m_node(NULL),
  m_enabled(false) {

}


/*
 *
 */
SandNetDevice::~SandNetDevice() {
  if (m_enabled)
    stop();
}


/*
 * Start this device
 *
 */
int SandNetDevice::start() {
  SandNetPort *port = NULL;
  int debug = 0;

  /* set up ports */
  for (int i=0; i < SANDNET_MAX_PORTS + INPUT_PORTS; i++) {
    port = new SandNetPort(this,i);

    if (port != NULL)
      this->add_port(port);
  }

#ifdef DEBUG
  debug = 1;
#endif

  // create new sandnet node, and set config values
  if (m_prefs->get_val("ip") == "")
    m_node = sandnet_new(NULL, debug);
  else {
    m_node = sandnet_new(m_prefs->get_val("ip").c_str(), debug);
  }

  if (!m_node) {
    OLA_WARN << "sandnet_new failed: " << sandnet_strerror();
    return -1;
  }

  // setup node
  if (sandnet_set_name(m_node, m_prefs->get_val("name").c_str()) ) {
    OLA_WARN << "sandnet_set_name failed: " << sandnet_strerror();
    goto e_sandnet_start;
  }

  // setup the output ports (ie INTO sandnet)
  for (int i =0; i < SANDNET_MAX_PORTS; i++) {
    if ( sandnet_set_port(m_node, i, SANDNET_PORT_MODE_IN, 0, i+1) ) {
      OLA_WARN << "sandnet_set_port failed: " << sandnet_strerror();
      goto e_sandnet_start;
    }
  }

  // we want to be notified when we recv dmx
  if (sandnet_set_dmx_handler(m_node, ::dmx_handler, (void*) this) ) {
    OLA_WARN << "sandnet_set_dmx_handler failed: " << sandnet_strerror();
    goto e_sandnet_start;
  }

  if (sandnet_start(m_node) ) {
    OLA_WARN << "sandnet_start failed: " << sandnet_strerror();
    goto e_sandnet_start;
  }

  m_enabled = true;
  return 0;

e_sandnet_start:
  if (sandnet_destroy(m_node))
    OLA_WARN << "sandnet_destory failed: " << sandnet_strerror();
  return -1;
}


/*
 * stop this device
 *
 */
int SandNetDevice::stop() {
  Port *prt = NULL;

  if (!m_enabled)
    return 0;

  for (int i=0; i < port_count(); i++) {
    prt = get_port(i);
    if (prt != NULL)
      delete prt;
  }

  if (sandnet_stop(m_node)) {
    OLA_WARN << "sandnet_stop failed: " << sandnet_strerror();
    return -1;
  }

  if (sandnet_destroy(m_node)) {
    OLA_WARN << "sandnet_destroy failed: " << sandnet_strerror();
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
sandnet_node SandNetDevice::get_node() const {
  return m_node;
}


/*
 * return the sd of this device
 *
 */
int SandNetDevice::get_sd(int i) const {
  int ret = sandnet_get_sd(m_node, i);
  if (ret < 0) {
    OLA_WARN << "sandnet_get_sd failed: " << sandnet_strerror();
    return -1;
  }
  return ret;
}


/*
 * Called when there is activity on our descriptors
 *
 * @param  data  user data (pointer to sandnet_device_priv
 */
int SandNetDevice::action() {

  if (sandnet_read(m_node, 0) ) {
    OLA_WARN << "sandnet_read failed: " << sandnet_strerror();
    return -1;
  }
  return 0;
}


/*
 * Called when there is activity on our descriptors
 *
 * @param  data  user data (pointer to sandnet_device_priv
 */
int SandNetDevice::timeout_action() {
  sandnet_send_ad(m_node);
  return 0;
}


// call this when something changes
// where to store data to ?
// I'm thinking a config file in /etc/olad/olad.conf
int SandNetDevice::save_config() const {


  return 0;
}


/*
 * we can pass plugin specific messages here
 * make sure the user app knows the format though...
 *
 */
int SandNetDevice::configure(void *req, int len) {
  // handle short/ long name & subnet and port addresses

  req = 0;
  len = 0;

  return 0;
}


/*
 * Add a port to our hash map
 */
int SandNetDevice::port_map(Universe *uni, SandNetPort *prt) {
  if (prt != NULL)
    sandnet_set_port(m_node, prt->get_id(), SANDNET_PORT_MODE_IN, 0, uni->get_uid());
  m_portmap[uni->get_uid()] = prt;
  return 0;
}

SandNetPort *SandNetDevice::get_port_from_uni(int uni) {
  return m_portmap[uni];
}
