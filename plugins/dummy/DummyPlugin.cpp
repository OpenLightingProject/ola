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
 * DummyPlugin.cpp
 * The Dummy plugin for lla, contains a single dummy device
 * Copyright (C) 2005-2007 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>

#include <llad/pluginadaptor.h>

#include "DummyPlugin.h"
#include "DummyDevice.h"

const string DummyPlugin::PLUGIN_NAME = "Dummy Plugin";
const string DummyPlugin::PLUGIN_PREFIX = "dummy";

/*
 * Entry point to this plugin
 */
extern "C" Plugin* create(const PluginAdaptor *pa) {
  return new DummyPlugin(pa, LLA_PLUGIN_DUMMY);
}

/*
 * Called when the plugin is unloaded
 */
extern "C" void destroy(Plugin *plug) {
  delete plug;
}


/*
 * Start the plugin
 *
 * Lets keep it simple, one device for this plugin
 */
int DummyPlugin::start_hook() {

  /* create new lla device */
  m_dev = new DummyDevice(this, "Dummy Device");

  if (m_dev == NULL)
    return -1;

  // start this device and register it
  m_dev->start();
  m_pa->register_device(m_dev);
  return 0;
}


/*
 * Stop the plugin
 *
 * @return 0 on sucess, -1 on failure
 */
int DummyPlugin::stop_hook() {
  // stop the device
  if (m_dev->stop())
    return -1;

  m_pa->unregister_device(m_dev);
  delete m_dev;
  return 0;
}


string DummyPlugin::get_desc() const {
  return
"Dummy Plugin\n"
"----------------------------\n"
"\n"
"The plugin creates a single device with one port. "
"When used as an output port it prints the first two bytes of dmx data to "
"stdout.\n";
}
