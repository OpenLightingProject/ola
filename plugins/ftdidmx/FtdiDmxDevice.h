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
 * FtdiDmxDevice.h
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 */

#ifndef PLUGINS_FTDIDMX_FTDIDMXDEVICE_H_
#define PLUGINS_FTDIDMX_FTDIDMXDEVICE_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Device.h"
#include "olad/Preferences.h"
#include "plugins/ftdidmx/FtdiWidget.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

using std::string;
using ola::Device;

class FtdiDmxDevice : public Device {
 public:
  FtdiDmxDevice(AbstractPlugin *owner,
                const FtdiWidgetInfo &widget_info,
                unsigned int frequency);
  ~FtdiDmxDevice();

  string DeviceId() const { return m_device->Serial(); }
  string Description() const { return m_widget_info.Description(); }

 protected:
  bool StartHook();

 private:
  auto_ptr<FtdiWidget> m_device;
  const FtdiWidgetInfo m_widget_info;
  unsigned int m_frequency;
};
}  // ftdidmx
}  // plugin
}  // ola
#endif  // PLUGINS_FTDIDMX_FTDIDMXDEVICE_H_
