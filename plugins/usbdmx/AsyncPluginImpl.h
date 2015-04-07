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
#endif

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ola/base/Macro.h"
#include "ola/thread/Mutex.h"
#include "ola/thread/Thread.h"
#include "plugins/usbdmx/PluginImplInterface.h"
#include "plugins/usbdmx/SyncronizedWidgetObserver.h"
#include "plugins/usbdmx/WidgetFactory.h"

namespace ola {

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
   */
  AsyncPluginImpl(PluginAdaptor *plugin_adaptor,
                  Plugin *plugin,
                  unsigned int debug_level);
  ~AsyncPluginImpl();

  bool Start();
  bool Stop();

  #ifdef HAVE_LIBUSB_HOTPLUG_API
  /**
   * @brief Called when a USB hotplug event occurs.
   * @param dev the libusb_device the event occurred for.
   * @param event indicates if the device was added or removed.
   *
   * This can be called from either the thread that called
   * Start(), or from the libusb thread. It can't be called from both threads at
   * once though, since the libusb thread is only started once the initial call
   * to libusb_hotplug_register_callback returns.
   */
  void HotPlugEvent(struct libusb_device *dev,
                    libusb_hotplug_event event);
  #endif

  bool NewWidget(class AnymauDMX *widget);
  bool NewWidget(class EurolitePro *widget);
  bool NewWidget(class JaRuleWidget *widget);
  bool NewWidget(class ScanlimeFadecandy *widget);
  bool NewWidget(class Sunlite *widget);
  bool NewWidget(class VellemanK8062 *widget);

  void WidgetRemoved(class AnymauDMX *widget);
  void WidgetRemoved(class EurolitePro *widget);
  void WidgetRemoved(class JaRuleWidget *widget);
  void WidgetRemoved(class ScanlimeFadecandy *widget);
  void WidgetRemoved(class Sunlite *widget);
  void WidgetRemoved(class VellemanK8062 *widget);

 private:
  typedef std::vector<class WidgetFactory*> WidgetFactories;
  typedef std::map<struct libusb_device*, WidgetFactory*> USBDeviceToFactoryMap;
  typedef std::map<class Widget*, Device*> WidgetToDeviceMap;
  typedef std::pair<uint8_t, uint8_t> USBDeviceID;
  typedef std::map<USBDeviceID, struct libusb_device*> USBDeviceIDs;

  PluginAdaptor* const m_plugin_adaptor;
  Plugin* const m_plugin;
  const unsigned int m_debug_level;

  SyncronizedWidgetObserver m_widget_observer;

  libusb_context *m_context;
  bool m_use_hotplug;
  ola::thread::Mutex m_mutex;
  bool m_suppress_hotplug_events;  // GUARDED_BY(m_mutex);
  std::auto_ptr<class LibUsbThread> m_usb_thread;
  std::auto_ptr<class AsyncronousLibUsbAdaptor> m_usb_adaptor;

  WidgetFactories m_widget_factories;
  USBDeviceToFactoryMap m_device_factory_map;
  WidgetToDeviceMap m_widget_device_map;

  // Members used if hotplug is not supported
  ola::thread::timeout_id m_scan_timeout;
  USBDeviceIDs m_seen_usb_devices;

  bool HotplugSupported();
  bool USBDeviceAdded(libusb_device *device);
  void USBDeviceRemoved(libusb_device *device);

  bool StartAndRegisterDevice(class Widget *widget, Device *device);
  void RemoveWidget(class Widget *widget);

  bool ScanUSBDevices();

  DISALLOW_COPY_AND_ASSIGN(AsyncPluginImpl);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_ASYNCPLUGINIMPL_H_
