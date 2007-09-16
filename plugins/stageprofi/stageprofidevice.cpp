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
 * stageprofidevice.cpp
 * StageProfi device
 * Copyright (C) 2006  Simon Newton
 *
 * The device creates two ports, one in and one out, but you can only use one at a time.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <llad/logger.h>
#include <llad/preferences.h>
#include <llad/universe.h>

#include "stageprofidevice.h"
#include "stageprofiport.h"
#include "StageProfiWidgetLan.h"
#include "StageProfiWidgetUsb.h"

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
StageProfiDevice::StageProfiDevice(Plugin *owner, const string &name, const string &dev_path) :
  Device(owner, name),
  m_path(dev_path),
  m_enabled(false),
  m_widget(NULL) {
    if (dev_path.c_str()[0] == '/') {
      m_widget = new StageProfiWidgetUsb();
    } else {
      m_widget = new StageProfiWidgetLan();
    }
}


/*
 * Destroy this device
 */
StageProfiDevice::~StageProfiDevice() {
  if (m_enabled)
    stop();

  if (m_widget != NULL)
    delete m_widget;
}


/*
 * Start this device
 */
int StageProfiDevice::start() {
  StageProfiPort *port = NULL;
  Port *prt = NULL;

  if (m_widget->connect(m_path)) {
    Logger::instance()->log(Logger::WARN, "StageProfiPlugin: failed to connect to %s", m_path.c_str());
    goto e_dev;
  }

  if (m_widget->detect_device()) {
    Logger::instance()->log(Logger::WARN, "StageProfiPlugin: no device found at %s", m_path.c_str());
    goto e_dev;
  }

  port = new StageProfiPort(this,0);

  if (port != NULL)
    this->add_port(port);

  m_enabled = true;
  return 0;

e_dev:
  delete prt;
  return -1;
}


/*
 * Stop this device
 */
int StageProfiDevice::stop() {
  Port *prt = NULL;
  Universe *uni;

  if (!m_enabled)
    return 0;

  // disconnect from widget
  m_widget->disconnect();

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
 * return the sd for this device
 */
int StageProfiDevice::get_sd() const {
  return m_widget->fd();
}


/*
 * Called when there is activity on our descriptors
 */
int StageProfiDevice::fd_action() {
  m_widget->recv();
  return 0;
}


/*
 * Send the dmx out the widget
 * called from the StageProfiPort
 *
 * @return   0 on success, non 0 on failure
 */
int StageProfiDevice::send_dmx(uint8_t *data, int len) {
  return m_widget->send_dmx(data,len);
}


// call this when something changes
// where to store data to ?
// I'm thinking a config file in /etc/llad/llad.conf
int StageProfiDevice::save_config() const {

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
LlaDevConfMsg *StageProfiDevice::configure(const uint8_t *req, int l) {
  req = NULL;
  l = 0;
  return NULL;
}
