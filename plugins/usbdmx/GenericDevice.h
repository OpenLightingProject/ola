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
 * GenericDevice.h
 * A Generic device that creates a single port.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_GENERICDEVICE_H_
#define PLUGINS_USBDMX_GENERICDEVICE_H_

#include <memory>
#include <string>
#include "ola/base/Macro.h"
#include "olad/Device.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief A Generic device.
 *
 * This simple generic device creates a single output port around a Widget.
 */
class GenericDevice: public Device {
 public:
  /**
   * @brief Create a new GenericDevice.
   * @param owner The plugin this device belongs to
   * @param widget The widget to use for this device.
   * @param device_name The name of the device.
   * @param device_id The id of the device.
   */
  GenericDevice(ola::AbstractPlugin *owner,
                class WidgetInterface *widget,
                const std::string &device_name,
                const std::string &device_id);

  std::string DeviceId() const {
    return m_device_id;
  }

 protected:
  bool StartHook();

 private:
  const std::string m_device_id;
  std::auto_ptr<class GenericOutputPort> m_port;

  DISALLOW_COPY_AND_ASSIGN(GenericDevice);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_GENERICDEVICE_H_
