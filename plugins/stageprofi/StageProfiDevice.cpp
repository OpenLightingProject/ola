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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * StageProfiDevice.cpp
 * StageProfi device
 * Copyright (C) 2006 Simon Newton
 */

#include "plugins/stageprofi/StageProfiDevice.h"

#include <string>

#include "ola/Logging.h"
#include "plugins/stageprofi/StageProfiPort.h"
#include "plugins/stageprofi/StageProfiWidget.h"

namespace ola {
namespace plugin {
namespace stageprofi {

using ola::AbstractPlugin;
using ola::io::ConnectedDescriptor;
using std::string;


StageProfiDevice::StageProfiDevice(AbstractPlugin *owner,
                                   StageProfiWidget *widget,
                                   const string &name)
    : Device(owner, name),
      m_widget(widget) {
}


StageProfiDevice::~StageProfiDevice() {
  // Stub destructor for compatibility with StageProfiWidget subclasses
}

string StageProfiDevice::DeviceId() const {
  return m_widget->GetPath();
}

bool StageProfiDevice::StartHook() {
  if (!m_widget.get())
    return false;

  StageProfiOutputPort *port = new StageProfiOutputPort(
      this, 0, m_widget.get());
  AddPort(port);
  return true;
}
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
