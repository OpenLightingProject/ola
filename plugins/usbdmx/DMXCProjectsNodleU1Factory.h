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
 * DMXCProjectsNodleU1Factory.h
 * The WidgetFactory for Nodle widgets.
 * Copyright (C) 2015 Stefan Krupop
 */

#ifndef PLUGINS_USBDMX_DMXCPROJECTSNODLEU1FACTORY_H_
#define PLUGINS_USBDMX_DMXCPROJECTSNODLEU1FACTORY_H_

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/base/Macro.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/usbdmx/WidgetFactory.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Creates Nodle widgets.
 */
class DMXCProjectsNodleU1Factory :
    public BaseWidgetFactory<class DMXCProjectsNodleU1> {
 public:
  explicit DMXCProjectsNodleU1Factory(ola::usb::LibUsbAdaptor *adaptor,
                                      PluginAdaptor *plugin_adaptor,
                                      Preferences *preferences)
      : BaseWidgetFactory<class DMXCProjectsNodleU1>(
            "DMXCProjectsNodleU1Factory"),
        m_adaptor(adaptor),
        m_plugin_adaptor(plugin_adaptor),
        m_preferences(preferences) {
  }

  bool DeviceAdded(
      WidgetObserver *observer,
      libusb_device *usb_device,
      const struct libusb_device_descriptor &descriptor);

 private:
  ola::usb::LibUsbAdaptor* const m_adaptor;
  PluginAdaptor* const m_plugin_adaptor;
  Preferences* const m_preferences;

  static const uint16_t VENDOR_ID_DMXC_PROJECTS;
  static const uint16_t VENDOR_ID_DE;
  static const uint16_t VENDOR_ID_FX5;
  static const uint16_t PRODUCT_ID_DMXC_P_NODLE_U1;
  static const uint16_t PRODUCT_ID_DE_USB_DMX;
  static const uint16_t PRODUCT_ID_FX5_DMX;

  DISALLOW_COPY_AND_ASSIGN(DMXCProjectsNodleU1Factory);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_DMXCPROJECTSNODLEU1FACTORY_H_
