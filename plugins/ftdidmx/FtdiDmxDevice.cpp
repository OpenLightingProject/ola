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
 * FtdiDmxDevice.cpp
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 */

#include <string>
#include <memory>
#include "ola/Logging.h"
#include "plugins/ftdidmx/FtdiDmxDevice.h"
#include "plugins/ftdidmx/FtdiDmxPort.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

using std::string;

FtdiDmxDevice::FtdiDmxDevice(AbstractPlugin *owner,
                             const FtdiWidgetInfo &widget_info,
                             unsigned int frequency)
    : Device(owner, widget_info.Description()),
      m_widget_info(widget_info),
      m_frequency(frequency) {
  m_widget.reset(
      new FtdiWidget(widget_info.Serial(),
                     widget_info.Name(),
                     widget_info.Id()));
}

FtdiDmxDevice::~FtdiDmxDevice() {
  if (m_widget->IsOpen())
    m_widget->Close();
}

bool FtdiDmxDevice::StartHook() {
  AddPort(new FtdiDmxOutputPort(this,
                                m_widget.get(),
                                m_widget_info.Id(),
                                m_frequency));
  return true;
}
}  // ftdidmx
}  // plugin
}  // ola
