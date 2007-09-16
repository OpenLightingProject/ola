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
 * dmx4linuxdevice.cpp
 * Dmx4Linux device
 * Copyright (C) 2006-2007 Simon Newton
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <llad/logger.h>
#include <llad/preferences.h>
#include <llad/universe.h>

#include "Dmx4LinuxPlugin.h"
#include "Dmx4LinuxDevice.h"
#include "Dmx4LinuxPort.h"

#if HAVE_CONFIG_H
#  include <config.h>
#endif

/*
 * Create a new device
 *
 * @param owner  the plugin that owns this device
 * @param name  the device name
 * @param dev_path  path to the pro widget
 */
Dmx4LinuxDevice::Dmx4LinuxDevice(Dmx4LinuxPlugin *owner, const string &name) :
  Device(owner, name),
  m_plugin(owner),
  m_enabled(false) {
}


/*
 * Destroy this device
 */
Dmx4LinuxDevice::~Dmx4LinuxDevice() {
  if (m_enabled)
    stop();
}


/*
 * Start this device
 */
int Dmx4LinuxDevice::start() {
  m_enabled = true;
  return 0;
}


/*
 * Stop this device
 * TODO: move into base class
 */
int Dmx4LinuxDevice::stop() {
  Port *prt = NULL;
  Universe *uni;

  if (!m_enabled)
    return 0;

  for (int i=0; i < port_count(); i++) {
    prt = get_port(i);
    if (prt != NULL) {
      uni = prt->get_universe();

      if (uni)
        uni->remove_port(prt);

      delete prt;
    }
  }

  m_enabled = false;
  return 0;
}


/*
 * Send the dmx out the widget
 * called from the Dmx4LinuxPort
 *
 * @return   0 on success, non 0 on failure
 */
int Dmx4LinuxDevice::send_dmx(int d4l_uni, uint8_t *data, int len) {
  return m_plugin->send_dmx(d4l_uni, data, len);
}


// call this when something changes
// where to store data to ?
int Dmx4LinuxDevice::save_config() const {
  return 0;
}


/*
 * This device doesn't support configuring...
 *
 * @param req    the request data
 * @param reql    the request length
 *
 * @return  the length of the reply
 */
LlaDevConfMsg *Dmx4LinuxDevice::configure(const uint8_t *req, int l) {
  req = NULL;
  l = 0;
  return NULL;
}
