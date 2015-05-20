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
 * A Ja Rule device that creates a single port.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef PLUGINS_USBDMX_JARULEDEVICE_H_
#define PLUGINS_USBDMX_JARULEDEVICE_H_

#include <memory>
#include <string>
#include "ola/base/Macro.h"
#include "olad/Device.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief A JaRule device.
 *
 * Each Ja Rule device has a single output port.
 */
class JaRuleDevice: public Device {
 public:
  /**
   * @brief Create a new JaRuleDevice.
   * @param owner The plugin this device belongs to
   * @param widget The Ja Rule widget to use for this device.
   * @param device_name The name of the device.
   * @param device_id The id of the device.
   */
  JaRuleDevice(ola::AbstractPlugin *owner,
               class JaRuleWidget *widget,
               const std::string &device_name,
               const std::string &device_id);

  std::string DeviceId() const {
    return m_device_id;
  }

 protected:
  bool StartHook();

 private:
  const std::string m_device_id;
  std::auto_ptr<class JaRuleOutputPort> m_port;

  DISALLOW_COPY_AND_ASSIGN(JaRuleDevice);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_JARULEDEVICE_H_
