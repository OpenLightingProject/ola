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
 * E131Device.cpp
 * E1.31 device
 * Copyright (C) 2007 Simon Newton
 *
 * Ids 0-3 : Input ports (recv dmx)
 * Ids 4-7 : Output ports (send dmx)
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "E131Device.h"
#include "E131Port.h"

#include <llad/logger.h>
#include <llad/preferences.h>

#include <acn/NetServer.h>
#include <acn/E131Node.h>
#include <acn/E131DmpLayer.h>

#if HAVE_CONFIG_H
#  include <config.h>
#endif

/*
 * called every iteration through the select loop
 */
void process_data(void *data) {
  E131DmpLayer *l = (E131DmpLayer*) data;
  if (l)
    l->process();
  return;
}

/*
 * Create a new device
 *
 * should prob pass the ip to bind to
 *
 */
E131Device::E131Device(Plugin *owner, const string &name, NetServer *ns, Preferences *prefs) :
  Device(owner, name),
  m_prefs(prefs),
  m_ns(ns),
  m_node(NULL),
  m_layer(NULL),
  m_enabled(false) {

}


/*
 *
 */
E131Device::~E131Device() {
  if (m_enabled)
    stop();

  delete m_node;
}


/*
 * Start this device
 *
 */
int E131Device::start() {
  struct in_addr addr;

  // decide what local address to use
  string ip_addr = m_prefs->get_val("address");
  if (ip_addr != "") {
    if (inet_aton(ip_addr.c_str(), &addr) == 0) {
      Logger::instance()->log(Logger::WARN, "E131Plugin: %s isn't a valid address", ip_addr.c_str());
      return -1;
    }
  } else
    addr.s_addr = htonl(INADDR_ANY);

  m_node = new E131Node(m_ns, addr);
  m_layer = m_node->init();

  E131Port *port = NULL ;

  /* set up ports */
  for(int i=0; i < 2 * E131Port::NUMB_PORTS; i++) {
    port = new E131Port(this, i, m_layer) ;

    if(port != NULL)
      this->add_port(port) ;
  }

  m_ns->loop_callback(process_data, (void*) m_layer);
  m_enabled = true ;

  return 0;

}


/*
 * stop this device
 *
 */
int E131Device::stop() {
  Port *prt = NULL;

  if (!m_enabled)
    return 0 ;

  for(int i=0; i < port_count() ; i++) {
    prt = get_port(i) ;
    if(prt != NULL)
      delete prt ;
  }

  m_enabled = false ;

  return 0;
}


/*
 * Called when there is activity on our descriptors
 *
 * @param  data  user data (pointer to artnet_device_priv
 */
int E131Device::fd_action() {
 /*
  if( artnet_read(m_node, 0) ) {
    Logger::instance()->log(Logger::WARN, "E131Plugin: artnet_read failed: %s", artnet_strerror()) ;
    return -1 ;
  }
  */
  return 0;
}


// call this when something changes
// where to store data to ?
// I'm thinking a config file in /etc/llad/llad.conf
int E131Device::save_config() const {


  return 0;
}



/*
 * we can pass plugin specific messages here
 * make sure the user app knows the format though...
 *
 */
class LlaDevConfMsg *E131Device::configure(const uint8_t *req, int len) {
  // use this to set source priority, name etc
  req++;
  len = 0;

  return 0;
}
