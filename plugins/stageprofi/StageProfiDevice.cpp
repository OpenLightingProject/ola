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
 * Copyright (C) 2006-2009 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <string>

#include "ola/Logging.h"
#include "olad/Preferences.h"
#include "olad/Universe.h"
#include "plugins/stageprofi/StageProfiDevice.h"
#include "plugins/stageprofi/StageProfiPort.h"
#include "plugins/stageprofi/StageProfiWidgetLan.h"
#include "plugins/stageprofi/StageProfiWidgetUsb.h"

namespace ola {
namespace plugin {
namespace stageprofi {

using ola::AbstractPlugin;

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
  if (m_widget)
    delete m_widget;
}


/*
 * Start this device
 */
bool StageProfiDevice::StartHook() {
  if (!m_widget)
    return false;

  if (!m_widget->Connect(m_path)) {
    OLA_WARN << "StageProfiPlugin: failed to connect to " << m_path;
    return false;
  }

  if (!m_widget->DetectDevice()) {
    OLA_WARN << "StageProfiPlugin: no device found at " << m_path;
    return false;
  }

  StageProfiOutputPort *port = new StageProfiOutputPort(this, 0, m_widget);
  AddPort(port);
  return true;
}


/*
 * Stop this device
 */
void StageProfiDevice::PrePortStop() {
  // disconnect from widget
  m_widget->Disconnect();
}


/*
 * return the sd for this device
 */
ConnectedSocket *StageProfiDevice::GetSocket() const {
  return m_widget->GetSocket();
}
}  // stageprofi
}  // plugin
}  // ola
