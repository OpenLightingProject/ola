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
 * pathportplugin.cpp
 * The Pathport plugin for lla
 * Copyright (C) 2005-2007 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>

#include <llad/pluginadaptor.h>
#include <llad/preferences.h>

#include "PathportPlugin.h"
#include "PathportDevice.h"

#define PATHPORT_NAME "lla-Pathport"

/*
 * Entry point to this plugin
 */
extern "C" Plugin* create(const PluginAdaptor *pa) {
  return new PathportPlugin(pa, LLA_PLUGIN_PATHPORT);
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
int PathportPlugin::start() {

  if (m_enabled)
    return -1;

  // setup prefs
  if (load_prefs() != 0)
    return -1;

  /* create new lla device */
  m_dev = new PathportDevice(this, "Pathport Device", m_prefs);

  if (m_dev == NULL)
    goto e_prefs;

  if (m_dev->start())
    goto e_dev;

  // register our descriptors
  for (int i = 0; i < PATHPORT_MAX_SD; i++)
    m_pa->register_fd( m_dev->get_sd(i), PluginAdaptor::READ, m_dev);

  m_pa->register_device(m_dev);

  m_enabled = true;
  return 0;

  e_dev:
    delete m_dev;
  e_prefs:
    delete m_prefs;
    return -1;

}


/*
 * Stop the plugin
 *
 * @return 0 on sucess, -1 on failure
 */
int PathportPlugin::stop() {

  if (!m_enabled)
    return -1;

  for (int i = 0; i < PATHPORT_MAX_SD; i++)
    m_pa->unregister_fd( m_dev->get_sd(i), PluginAdaptor::READ);

  // stop the device
  if (m_dev->stop())
    return -1;


  m_pa->unregister_device(m_dev);
  m_enabled = false;
  delete m_dev;
  delete m_prefs;
  return 0;
}

/*
 * return the description for this plugin
 *
 */
string PathportPlugin::get_desc() const {
  return
"Pathport Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates a single device with 5 input and 5 output ports.\n"
"\n"
"The universe the port is patched to corresponds with the DMX channels used in the pathport protocol. "
"For example universe 0 is xDMX channels 1 - 512, universe 1 is xDMX channels 513 - 1024.\n"
"\n"
"--- Config file : lla-pathport.conf ---\n"
"\n"
"ip = a.b.c.d\n"
"The ip address to bind to. If not specified it will use the first non-loopback ip.\n"
"\n"
"name = lla-Pathport\n"
"The name of the node.\n";

}


/*
 * load the plugin prefs and default to sensible values
 *
 */
int PathportPlugin::load_prefs() {
  if (m_prefs != NULL)
    delete m_prefs;

  m_prefs = new Preferences("pathport");

  if (m_prefs == NULL)
    return -1;

  m_prefs->load();

  // we don't worry about ip here
  // if it's non existant it will choose one
  if ( m_prefs->get_val("name") == "") {
    m_prefs->set_val("name", PATHPORT_NAME);
    m_prefs->save();
  }

  // check if this save correctly
  // we don't want to use it if null
  if ( m_prefs->get_val("name") == "" ) {
    delete m_prefs;
    return -1;
  }

  return 0;
}
