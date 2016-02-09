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
 * DMXCProjectsNodleU1Device.h
 * A DMXCProjectsNodleU1 device that creates an input and an output port.
 * Copyright (C) 2015 Stefan Krupop
 */

#ifndef PLUGINS_USBDMX_DMXCPROJECTSNODLEU1DEVICE_H_
#define PLUGINS_USBDMX_DMXCPROJECTSNODLEU1DEVICE_H_

#include <memory>
#include <string>
#include "ola/base/Macro.h"
#include "olad/Device.h"
#include "plugins/usbdmx/DMXCProjectsNodleU1.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief A DMXCProjectsNodleU1 device.
 *
 * This device creates an output and/or input port around a Widget.
 */
class DMXCProjectsNodleU1Device: public Device {
 public:
  /**
   * @brief Create a new DMXCProjectsNodleU1Device.
   * @param owner The plugin this device belongs to
   * @param widget The widget to use for this device.
   * @param device_name The name of the device.
   * @param device_id The id of the device.
   * @param plugin_adaptor a PluginAdaptor object, used by the input port.
   */
  DMXCProjectsNodleU1Device(ola::AbstractPlugin *owner,
                            class DMXCProjectsNodleU1 *widget,
                            const std::string &device_name,
                            const std::string &device_id,
                            PluginAdaptor *plugin_adaptor);

  std::string DeviceId() const {
    return m_device_id;
  }

 protected:
  bool StartHook();

 private:
  const std::string m_device_id;
  std::auto_ptr<class GenericOutputPort> m_out_port;
  std::auto_ptr<class DMXCProjectsNodleU1InputPort> m_in_port;

  DISALLOW_COPY_AND_ASSIGN(DMXCProjectsNodleU1Device);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_DMXCPROJECTSNODLEU1DEVICE_H_
