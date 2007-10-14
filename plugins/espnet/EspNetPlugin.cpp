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
 *
 * EspNetPlugin.cpp
 * The Esp Net plugin for lla
 * Copyright (C) 2005  Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>

#include <llad/pluginadaptor.h>
#include <llad/preferences.h>

#include "EspNetPlugin.h"
#include "EspNetDevice.h"

const string EspNetPlugin::ESPNET_NODE_NAME = "lla-EspNet";
const string EspNetPlugin::ESPNET_DEVICE_NAME = "EspNet Device";
const string EspNetPlugin::PLUGIN_NAME = "EspNet Plugin";
const string EspNetPlugin::PLUGIN_PREFIX = "espnet";

/*
 * Entry point to this plugin
 */
extern "C" Plugin* create(const PluginAdaptor *pa) {
  return new EspNetPlugin(pa, LLA_PLUGIN_ESPNET);
}

/*
 * Called when the plugin is unloaded
 */
extern "C" void destroy(Plugin* plug) {
  delete plug;
}


/*
 * Start the plugin
 *
 * For now we just have one device.
 */
int EspNetPlugin::start_hook() {
  /* create new lla device */
  m_dev = new EspNetDevice(this, ESPNET_DEVICE_NAME, m_prefs);

  if(m_dev == NULL)
    return -1;

  if(m_dev->start()) {
    delete m_dev;
    return -1;
  }

  // register our descriptors
  m_pa->register_fd( m_dev->get_sd(), PluginAdaptor::READ, m_dev);
  m_pa->register_device(m_dev);

  return 0;
}


/*
 * Stop the plugin
 *
 * @return 0 on sucess, -1 on failure
 */
int EspNetPlugin::stop_hook() {
  m_pa->unregister_fd( m_dev->get_sd(), PluginAdaptor::READ) ;

  if (m_dev->stop())
    return -1;

  m_pa->unregister_device(m_dev);
  delete m_dev;
  return 0;
}

/*
 * return the description for this plugin
 *
 */
string EspNetPlugin::get_desc() const {
  return
"EspNet Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates a single device with five input and five output ports.\n"
"\n"
"Esp supports up to 255 universes. As ESP has no notion of ports, we provide "
"a fixed number of ports which can be patched to any universe. When sending "
"data from a port, the data is addressed to the universe the port is patched "
"to. For example if port 0 is patched to universe 10, the data will be sent to "
"ESP universe 10.\n"
"\n"
"--- Config file : lla-espnet.conf ---\n"
"\n"
"ip = a.b.c.d\n"
"The ip address to bind to. If not specified it will use the first non-loopback ip.\n"
"\n"
"name = lla-EspNet\n"
"The name of the node.\n";

}

/*
 * load the plugin prefs and default to sensible values
 *
 */
int EspNetPlugin::set_default_prefs() {
  if (m_prefs == NULL)
    return -1;

  // we don't worry about ip here
  // if it's non existant it will choose one
  if (m_prefs->get_val("name") == "") {
    m_prefs->set_val("name", ESPNET_NODE_NAME);
    m_prefs->save();
  }

  // check if this save correctly
  // we don't want to use it if null
  if (m_prefs->get_val("name") == "")
    return -1;

  return 0;
}
