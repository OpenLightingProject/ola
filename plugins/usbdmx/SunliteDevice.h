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
 * SunliteDevice.h
 * The Sunlite USBDMX2 device.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBDMX_SUNLITEDEVICE_H_
#define PLUGINS_USBDMX_SUNLITEDEVICE_H_

#include <memory>
#include <string>
#include "ola/base/Macro.h"
#include "olad/Device.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief A Sunlite device
 */
class SunliteDevice: public Device {
 public:
  SunliteDevice(ola::AbstractPlugin *owner,
                class SunliteWidget *widget);

  std::string DeviceId() const { return "usbdmx2"; }

 protected:
  bool StartHook();

 private:
  std::auto_ptr<class GenericOutputPort> m_port;

  DISALLOW_COPY_AND_ASSIGN(SunliteDevice);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_SUNLITEDEVICE_H_
