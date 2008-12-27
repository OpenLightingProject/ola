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
 * StageProfiDevice.cpp
 * StageProfi device
 * Copyright (C) 2006-2007 Simon Newton
 *
 * The device creates two ports, one in and one out, but you can only use one at a time.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <llad/logger.h>
#include <llad/Preferences.h>
#include <llad/Universe.h>

#include "StageProfiDevice.h"
#include "StageProfiPort.h"
#include "StageProfiWidgetLan.h"
#include "StageProfiWidgetUsb.h"

#if HAVE_CONFIG_H
#  include <config.h>
#endif

namespace lla {
namespace plugin {

using lla::AbstractPlugin;

/*
 * Create a new device
 *
 * @param owner  the plugin that owns this device
 * @param name  the device name
 * @param dev_path  path to the pro widget
 */
StageProfiDevice::StageProfiDevice(AbstractPlugin *owner,
                                   const string &name,
                                   const string &dev_path):
  Device(owner, name),
  m_path(dev_path),
  m_enabled(false),
  m_widget(NULL) {
    if (dev_path.at(0) == '/') {
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
    Stop();

  if (m_widget)
    delete m_widget;
}


/*
 * Start this device
 */
bool StageProfiDevice::Start() {
  if (m_enabled || !m_widget)
    return false;

  if (!m_widget->Connect(m_path)) {
    Logger::instance()->log(Logger::WARN, "StageProfiPlugin: failed to connect to %s", m_path.c_str());
    return false;
  }

  if (!m_widget->DetectDevice()) {
    Logger::instance()->log(Logger::WARN, "StageProfiPlugin: no device found at %s", m_path.c_str());
    return false;
  }

  StageProfiPort *port = new StageProfiPort(this, 0);

  if (!port)
    return false;

  AddPort(port);
  m_enabled = true;
  return true;
}


/*
 * Stop this device
 */
bool StageProfiDevice::Stop() {
  if (!m_enabled)
    return true;

  // disconnect from widget
  m_widget->Disconnect();
  DeleteAllPorts();

  m_enabled = false;
  return true;
}


/*
 * return the sd for this device
 */
Socket *StageProfiDevice::GetSocket() const {
  return m_widget->GetSocket();
}


/*
 * Send the dmx out the widget
 * called from the StageProfiPort
 *
 * @return   0 on success, non 0 on failure
 */
int StageProfiDevice::SendDmx(uint8_t *data, int len) {
  return m_widget->SendDmx(data,len);
}

} // plugin
} //lla
