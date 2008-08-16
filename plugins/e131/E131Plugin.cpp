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
 * E131Plugin.cpp
 * The E1.31 plugin for lla
 * Copyright (C) 2007 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>

#include <llad/pluginadaptor.h>
#include <llad/preferences.h>

#include "E131Plugin.h"
#include "E131Device.h"
#include "LlaNetServer.h"

const string E131Plugin::PLUGIN_NAME = "E131 Plugin";
const string E131Plugin::PLUGIN_PREFIX = "e131";

/*
 * Entry point to this plugin
 */
extern "C" Plugin* create(const PluginAdaptor *pa) {
  return new E131Plugin(pa, LLA_PLUGIN_E131);
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
int E131Plugin::start_hook() {
  m_ns = new LlaNetServer(m_pa);

  /* create new lla device */
  m_dev = new E131Device(this, "E131 Device", m_ns, m_prefs);

  if (m_dev == NULL)
    return -1;

  if (m_dev->start()) {
    delete m_dev;
    return -1;
  }

  m_pa->register_device(m_dev);
  return 0;
}


/*
 * Stop the plugin
 *
 * @return 0 on sucess, -1 on failure
 */
int E131Plugin::stop_hook() {
  if (m_dev != NULL) {

    // stop the device
    if (m_dev->stop())
      return -1;

    m_pa->unregister_device(m_dev);
    delete m_dev;
  }

  delete m_ns;
  return 0;
}

/*
 * return the description for this plugin
 *
 */
string E131Plugin::get_desc() const {
    return
"E131 Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates a single device with eight input and eight output ports.\n"
"\n"
"Each port can be assigned to a diffent E1.31 Universe.\n"
"\n"
"--- Config file : lla-e131.conf ---\n"
"\n"
"ip = a.b.c.d\n"
"The local ip address to use for multicasting.\n"
"\n";
}
