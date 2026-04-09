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
 * AsyncPluginImpl.h
 * The asynchronous libusb implementation.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_ASYNCPLUGINIMPL_H_
#define PLUGINS_USBDMX_ASYNCPLUGINIMPL_H_

#include <libusb.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "libs/usb/Types.h"
#include "libs/usb/HotplugAgent.h"

#include "ola/base/Macro.h"
#include "olad/Preferences.h"
#include "plugins/usbdmx/PluginImplInterface.h"
#include "plugins/usbdmx/SynchronizedWidgetObserver.h"
#include "plugins/usbdmx/WidgetFactory.h"

namespace ola {
namespace usb {
class LibUsbThread;
class AsynchronousLibUsbAdaptor;
}  // namespace usb

class Device;

namespace plugin {
namespace usbdmx {

/**
 * @brief The asynchronous libusb implementation.
 */
class AsyncPluginImpl: public PluginImplInterface, public WidgetObserver {
 public:
  /**
   * @brief Create a new AsyncPluginImpl.
   * @param plugin_adaptor The PluginAdaptor to use, ownership is not
   * transferred.
   * @param plugin The parent Plugin object which is used when creating
   * devices.
   * @param debug_level the debug level to use for libusb.
   * @param preferences The Preferences container used by the plugin
   */
  AsyncPluginImpl(PluginAdaptor *plugin_adaptor,
                  Plugin *plugin,
                  unsigned int debug_level,
                  Preferences *preferences);
  ~AsyncPluginImpl();

  bool Start();
  bool Stop();

  // These are all run in the main SelectServer thread.
  bool NewWidget(class AnymauDMX *widget);
  bool NewWidget(class AVLdiyD512 *widget);
  bool NewWidget(class DMXCProjectsNodleU1 *widget);
  bool NewWidget(class DMXCreator512Basic *widget);
  bool NewWidget(class EurolitePro *widget);
  bool NewWidget(ola::usb::JaRuleWidget *widget);
  bool NewWidget(class ScanlimeFadecandy *widget);
  bool NewWidget(class ShowJockeyDMXU1 *widget);
  bool NewWidget(class Sunlite *widget);
  bool NewWidget(class VellemanK8062 *widget);

 private:
  typedef std::vector<class WidgetFactory*> WidgetFactories;
  typedef std::map<ola::usb::USBDeviceID, class DeviceState*> USBDeviceMap;

  PluginAdaptor* const m_plugin_adaptor;
  Plugin* const m_plugin;
  const unsigned int m_debug_level;
  std::auto_ptr<ola::usb::HotplugAgent> m_agent;
  Preferences* const m_preferences;

  SynchronizedWidgetObserver m_widget_observer;
  ola::usb::AsynchronousLibUsbAdaptor *m_usb_adaptor;  // not owned
  WidgetFactories m_widget_factories;
  USBDeviceMap m_device_map;

  void DeviceEvent(ola::usb::HotplugAgent::EventType event,
                   struct libusb_device *device);
  void SetupUSBDevice(libusb_device *device);

  template <typename Widget>
  bool StartAndRegisterDevice(Widget *widget, Device *device);

  void ShutdownDeviceState(DeviceState *state);

  DISALLOW_COPY_AND_ASSIGN(AsyncPluginImpl);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_ASYNCPLUGINIMPL_H_
