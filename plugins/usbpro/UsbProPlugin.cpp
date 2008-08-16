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
 * UsbProPlugin.cpp
 * The UsbPro plugin for lla
 * Copyright (C) 2006-2007 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>

#include <llad/pluginadaptor.h>
#include <llad/preferences.h>
#include <llad/logger.h>

#include "UsbProPlugin.h"
#include "UsbProDevice.h"

#include <vector>

const string UsbProPlugin::USBPRO_DEVICE_PATH = "/dev/ttyUSB0";
const string UsbProPlugin::USBPRO_DEVICE_NAME = "Enttec Usb Pro Device";
const string UsbProPlugin::PLUGIN_NAME = "UsbPro Plugin";
const string UsbProPlugin::PLUGIN_PREFIX = "usbpro";

/*
 * Entry point to this plugin
 */
extern "C" Plugin* create(const PluginAdaptor *pa) {
  return new UsbProPlugin(pa, LLA_PLUGIN_USBPRO);
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
 * Multiple devices now supported
 */
int UsbProPlugin::start_hook() {
  int sd;
  vector<string> *dev_nm_v;
  vector<string>::iterator it;
  UsbProDevice *dev;

  // fetch device listing
  dev_nm_v = m_prefs->get_multiple_val("device");

  // for each device
  for (it = dev_nm_v->begin(); it != dev_nm_v->end(); ++it) {

    /* create new lla device */
    dev = new UsbProDevice(this, USBPRO_DEVICE_NAME, *it);

    if (dev == NULL)
      continue;

    if (dev->start()) {
      delete dev;
      continue;
    }

    // register our descriptors, with us as the manager
    // this should really be fatal
    if ((sd = dev->get_sd()) >= 0)
      m_pa->register_fd( sd, PluginAdaptor::READ, dev, this);

    m_pa->register_device(dev);

    m_devices.insert(m_devices.end(), dev);

  }

  delete dev_nm_v;
  return 0;
}


/*
 * Stop the plugin
 *
 * @return 0 on sucess, -1 on failure
 */
int UsbProPlugin::stop_hook() {
  UsbProDevice *dev;
  unsigned int i = 0;

  for (i = 0; i < m_devices.size(); i++) {
    dev = m_devices[i];

    m_pa->unregister_fd( dev->get_sd(), PluginAdaptor::READ);

    // stop the device
    if (dev->stop())
      continue;

    m_pa->unregister_device(dev);
    delete dev;
  }

  m_devices.clear();
  return 0;
}

/*
 * return the description for this plugin
 *
 */
string UsbProPlugin::get_desc() const {
    return
"Enttec Usb Pro Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates devices with one input and one output port.\n"
"\n"
"--- Config file : lla-usbpro.conf ---\n"
"\n"
"device = /dev/ttyUSB0\n"
"The device to use. Multiple devices are allowed\n";
}

/*
 * Called if fd_action returns an error for one of our devices
 *
 */
int UsbProPlugin::fd_error(int error, Listener *listener) {
  UsbProDevice *dev  = dynamic_cast<UsbProDevice *> (listener);
  vector<UsbProDevice *>::iterator iter;

  if ( ! dev) {
    Logger::instance()->log(Logger::WARN, "fd_error : dynamic cast failed");
    return 0;
  }

  // stop this device
  m_pa->unregister_fd( dev->get_sd(), PluginAdaptor::READ);

  // stop the device
  dev->stop();

  m_pa->unregister_device(dev);

  iter = find(m_devices.begin() , m_devices.end(), dev);
  if (*iter == dev)
    m_devices.erase(iter);

  delete dev;
  error = 0;
  return 0;
}

/*
 * load the plugin prefs and default to sensible values
 *
 */
int UsbProPlugin::set_default_prefs() {
  if (m_prefs == NULL)
    return -1;

  if ( m_prefs->get_val("device") == "") {
    m_prefs->set_val("device", USBPRO_DEVICE_PATH);
    m_prefs->save();
  }

  // check if this saved correctly
  // we don't want to use it if null
  if (m_prefs->get_val("device") == "") {
    delete m_prefs;
    return -1;
  }
  return 0;
}
