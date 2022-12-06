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
 * FtdiDmxDevice.h
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 *
 * Additional modifications to enable support for multiple outputs and
 * additional device ids did change the original structure.
 *
 * by E.S. Rosenberg a.k.a. Keeper of the Keys 5774/2014
 */

#ifndef PLUGINS_FTDIDMX_FTDIDMXDEVICE_H_
#define PLUGINS_FTDIDMX_FTDIDMXDEVICE_H_

#include <string>
#include <memory>
#include "ola/DmxBuffer.h"
#include "olad/Device.h"
#include "olad/Preferences.h"
#include "plugins/ftdidmx/FtdiWidget.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

class FtdiDmxDevice : public Device {
 public:
  FtdiDmxDevice(AbstractPlugin *owner,
                const FtdiWidgetInfo &widget_info,
                unsigned int frequency);
  ~FtdiDmxDevice();

  std::string DeviceId() const { return m_widget->Serial(); }
  std::string Description() const { return m_widget_info.Description(); }
  FtdiWidget* GetDevice() { return m_widget; }

  // We can send the same universe to multiple ports, or patch port 2 before
  // port 1
  bool AllowMultiPortPatching() const { return true; }

 protected:
  bool StartHook();

 private:
  FtdiWidget *m_widget;
  const FtdiWidgetInfo m_widget_info;
  unsigned int m_frequency;
};
}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_FTDIDMX_FTDIDMXDEVICE_H_
