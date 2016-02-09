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
 * JaRuleDevice.h
 * A Ja Rule device.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef PLUGINS_USBDMX_JARULEDEVICE_H_
#define PLUGINS_USBDMX_JARULEDEVICE_H_

#include <set>
#include <string>

#include "libs/usb/JaRuleWidget.h"
#include "ola/base/Macro.h"
#include "olad/Device.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief A JaRule device, that represents one widget.
 *
 * A widget may have multiple input / output ports.
 */
class JaRuleDevice: public Device {
 public:
  /**
   * @brief Create a new JaRuleDevice.
   * @param owner The plugin this device belongs to.
   * @param widget An initalized JaRuleWidget.
   * @param device_name The name of the device.
   */
  JaRuleDevice(ola::AbstractPlugin *owner,
               ola::usb::JaRuleWidget *widget,
               const std::string &device_name);

  std::string DeviceId() const {
    return m_device_id;
  }

 protected:
  bool StartHook();

 private:
  ola::usb::JaRuleWidget *m_widget;  // not owned
  const std::string m_device_id;

  DISALLOW_COPY_AND_ASSIGN(JaRuleDevice);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_JARULEDEVICE_H_
