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
 * FtdiDmxDevice.cpp
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 */

#include <string>
#include "ola/Logging.h"
#include "plugins/ftdidmx/FtdiDmxDevice.h"
#include "plugins/ftdidmx/FtdiDmxPort.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

using std::string;

FtdiDmxDevice::FtdiDmxDevice(AbstractPlugin *owner,
                             FtdiWidgetInfo &devInfo,
                             Preferences *preferences) :
  Device(owner, devInfo.Description()),
  m_devInfo(devInfo),
  m_preferences(preferences) {
  auto_ptr<ola::plugin::ftdidmx::FtdiWidget>
    m_device(new FtdiWidget(devInfo.Serial(), devInfo.Name(), devInfo.Id()));
}

FtdiDmxDevice::~FtdiDmxDevice() {
  if (m_device->IsOpen()) m_device->Close();
}

bool FtdiDmxDevice::StartHook() {
  AddPort(new FtdiDmxOutputPort(this, m_devInfo.Id(), m_preferences));
  return true;
}
}
}
}
