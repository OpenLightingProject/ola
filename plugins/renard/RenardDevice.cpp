/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * RenardDevice.cpp
 * Renard device
 * Copyright (C) 2013 Hakan Lindestaf
 */

#include <string>

#include "ola/Logging.h"
#include "plugins/renard/RenardPort.h"
#include "plugins/renard/RenardWidgetSS24.h"

namespace ola {
namespace plugin {
namespace renard {

using ola::AbstractPlugin;

/*
 * Create a new device
 *
 * @param owner  the plugin that owns this device
 * @param name  the device name
 * @param dev_path  path to the pro widget
 */
RenardDevice::RenardDevice(AbstractPlugin *owner,
                             const string &name,
                             const string &dev_path)
    : Device(owner, name),
      m_path(dev_path) {
  // Currently always create a SS24 interface pending future options
  m_widget.reset(new RenardWidgetSS24(m_path));
}


/*
 * Destroy this device
 */
RenardDevice::~RenardDevice() {
  // Stub destructor for compatibility with RenardWidget subclasses
}


/*
 * Start this device
 */
bool RenardDevice::StartHook() {
  if (!m_widget.get())
    return false;

  if (!m_widget->Connect()) {
    OLA_WARN << "Failed to connect to " << m_path;
    return false;
  }

  if (!m_widget->DetectDevice()) {
    OLA_WARN << "No device found at " << m_path;
    return false;
  }

  RenardOutputPort *port = new RenardOutputPort(this, 0, m_widget.get());
  AddPort(port);
  return true;
}


/*
 * Stop this device
 */
void RenardDevice::PrePortStop() {
  // disconnect from widget
  m_widget->Disconnect();
}


/*
 * return the sd for this device
 */
ConnectedDescriptor *RenardDevice::GetSocket() const {
  return m_widget->GetSocket();
}
}  // namespace renard
}  // namespace plugin
}  // namespace ola
