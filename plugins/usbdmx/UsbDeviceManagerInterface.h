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
 * UsbDeviceManagerInterface.h
 * The interface for the USB Device Managers
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_USBDEVICEMANAGERINTERFACE_H_
#define PLUGINS_USBDMX_USBDEVICEMANAGERINTERFACE_H_

#include <libusb.h>
#include <map>
#include "ola/base/Macro.h"
#include "ola/stl/STLUtils.h"
#include "ola/Logging.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Manages a particular type of USB Device.
 */
class UsbDeviceManagerInterface {
 public:
  virtual ~UsbDeviceManagerInterface() {}

  virtual bool DeviceAdded(
      libusb_device *device,
      const struct libusb_device_descriptor &descriptor) = 0;

  virtual void DeviceRemoved(libusb_device *device) = 0;
};

template <typename DeviceClass>
class BaseDeviceFactory : public UsbDeviceManagerInterface {
 public:
  BaseDeviceFactory(ola::PluginAdaptor *plugin_adaptor, ola::Plugin *plugin)
    : m_plugin_adaptor(plugin_adaptor),
      m_plugin(plugin) {
  }

  void DeviceRemoved(libusb_device *device);

 protected:
  bool HasDevice(libusb_device *device) {
    return STLContains(m_device_map, device);
  }

  ola::Plugin* ParentPlugin() { return m_plugin; }

  bool RegisterDevice(libusb_device *usb_device, DeviceClass *device);

 private:
  typedef std::map<libusb_device*, DeviceClass*> DeviceMap;

  ola::PluginAdaptor *m_plugin_adaptor;
  ola::Plugin *m_plugin;
  DeviceMap m_device_map;

  DISALLOW_COPY_AND_ASSIGN(BaseDeviceFactory<DeviceClass>);
};

template <typename DeviceClass>
bool BaseDeviceFactory<DeviceClass>::RegisterDevice(libusb_device *usb_device,
                                                    DeviceClass *device) {
  if (!device->Start()) {
    delete device;
    return false;
  }

  DeviceClass *old_device = STLReplacePtr(&m_device_map, usb_device, device);
  if (old_device) {
    m_plugin_adaptor->UnregisterDevice(old_device);
    old_device->Stop();
    delete old_device;
  }
  m_plugin_adaptor->RegisterDevice(device);
  return true;
}

template <typename DeviceClass>
void BaseDeviceFactory<DeviceClass>::DeviceRemoved(libusb_device *usb_device) {
  OLA_INFO << "Removing device " << usb_device;
  DeviceClass *device = STLLookupAndRemovePtr(&m_device_map, usb_device);
  if (device) {
    m_plugin_adaptor->UnregisterDevice(device);
    device->Stop();
    delete device;
  }
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_USBDEVICEMANAGERINTERFACE_H_
