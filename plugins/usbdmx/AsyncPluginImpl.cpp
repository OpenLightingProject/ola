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
 * AsyncPluginImpl.cpp
 * The asynchronous libusb implementation.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/AsyncPluginImpl.h"

#include <stdlib.h>
#include <stdio.h>
#include <libusb.h>

#include <set>

#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"
#include "ola/strings/Format.h"
#include "olad/PluginAdaptor.h"

#include "plugins/usbdmx/AnymauDMX.h"
#include "plugins/usbdmx/AnymauDMXFactory.h"
#include "plugins/usbdmx/EuroliteProFactory.h"
#include "plugins/usbdmx/GenericDevice.h"
#include "plugins/usbdmx/JaRuleDevice.h"
#include "plugins/usbdmx/JaRuleFactory.h"
#include "plugins/usbdmx/JaRuleWidget.h"
#include "plugins/usbdmx/LibUsbAdaptor.h"
#include "plugins/usbdmx/LibUsbThread.h"
#include "plugins/usbdmx/ScanlimeFadecandy.h"
#include "plugins/usbdmx/ScanlimeFadecandyFactory.h"
#include "plugins/usbdmx/SunliteFactory.h"
#include "plugins/usbdmx/VellemanK8062.h"
#include "plugins/usbdmx/VellemanK8062Factory.h"

namespace ola {
namespace plugin {
namespace usbdmx {

namespace {

#ifdef HAVE_LIBUSB_HOTPLUG_API
/**
 * @brief Called by libusb when a USB device is added / removed.
 */
#ifdef _WIN32
__attribute__((__stdcall__))
#endif
int hotplug_callback(OLA_UNUSED struct libusb_context *ctx,
                     struct libusb_device *dev,
                     libusb_hotplug_event event,
                     void *user_data) {
  AsyncPluginImpl *plugin = reinterpret_cast<AsyncPluginImpl*>(user_data);
  plugin->HotPlugEvent(dev, event);
  return 0;
}
#endif
}  // namespace

AsyncPluginImpl::AsyncPluginImpl(PluginAdaptor *plugin_adaptor,
                                 Plugin *plugin,
                                 unsigned int debug_level)
    : m_plugin_adaptor(plugin_adaptor),
      m_plugin(plugin),
      m_debug_level(debug_level),
      m_widget_observer(this, plugin_adaptor),
      m_context(NULL),
      m_use_hotplug(false),
      m_suppress_hotplug_events(false),
      m_scan_timeout(ola::thread::INVALID_TIMEOUT) {
}

AsyncPluginImpl::~AsyncPluginImpl() {
  STLDeleteElements(&m_widget_factories);
}

bool AsyncPluginImpl::Start() {
  if (libusb_init(&m_context)) {
    OLA_WARN << "Failed to init libusb";
    return false;
  }

  OLA_DEBUG << "libusb debug level set to " << m_debug_level;
  libusb_set_debug(m_context, m_debug_level);

  m_use_hotplug = HotplugSupported();
  OLA_INFO << "HotplugSupported returned " << m_use_hotplug;
  if (m_use_hotplug) {
#ifdef HAVE_LIBUSB_HOTPLUG_API
    m_usb_thread.reset(new LibUsbHotplugThread(
          m_context, hotplug_callback, this));
#else
    OLA_FATAL << "Mismatch between m_use_hotplug and "
      " HAVE_LIBUSB_HOTPLUG_API";
    return false;
#endif
  } else {
    m_usb_thread.reset(new LibUsbSimpleThread(m_context));
  }
  m_usb_adaptor.reset(new AsyncronousLibUsbAdaptor(m_usb_thread.get()));

  // Setup the factories.
  m_widget_factories.push_back(new AnymauDMXFactory(m_usb_adaptor.get()));
  m_widget_factories.push_back(
      new EuroliteProFactory(m_usb_adaptor.get()));
  m_widget_factories.push_back(
      new JaRuleFactory(m_plugin_adaptor, m_usb_adaptor.get()));
  m_widget_factories.push_back(
      new ScanlimeFadecandyFactory(m_usb_adaptor.get()));
  m_widget_factories.push_back(new SunliteFactory(m_usb_adaptor.get()));
  m_widget_factories.push_back(new VellemanK8062Factory(m_usb_adaptor.get()));

  // If we're using hotplug, this starts the hotplug thread.
  if (!m_usb_thread->Init()) {
    STLDeleteElements(&m_widget_factories);
    m_usb_adaptor.reset();
    m_usb_thread.reset();
    return false;
  }

  if (!m_use_hotplug) {
    // Either we don't support hotplug or the setup failed.
    // As poor man's hotplug, we call libusb_get_device_list periodically to
    // check for new devices.
    m_scan_timeout = m_plugin_adaptor->RegisterRepeatingTimeout(
        TimeInterval(5, 0),
        NewCallback(this, &AsyncPluginImpl::ScanUSBDevices));

    // Call it immediately now.
    ScanUSBDevices();
  }

  return true;
}

bool AsyncPluginImpl::Stop() {
  if (m_scan_timeout != ola::thread::INVALID_TIMEOUT) {
    m_plugin_adaptor->RemoveTimeout(m_scan_timeout);
    m_scan_timeout = ola::thread::INVALID_TIMEOUT;
  }

  // The shutdown sequence is:
  //  - suppress hotplug events so we don't add any new devices
  //  - remove all existing devices
  //  - stop the usb_thread (if using hotplug, otherwise this is a noop).
  {
    ola::thread::MutexLocker locker(&m_mutex);
    m_suppress_hotplug_events = true;
  }

  USBDeviceToFactoryMap::iterator iter = m_device_factory_map.begin();
  for (; iter != m_device_factory_map.end(); ++iter) {
    iter->second->DeviceRemoved(this, iter->first);
  }
  m_device_factory_map.clear();

  m_usb_thread->Shutdown();

  m_usb_thread.reset();
  m_usb_adaptor.reset();
  STLDeleteElements(&m_widget_factories);

  libusb_exit(m_context);
  m_context = NULL;
  return true;
}

#ifdef HAVE_LIBUSB_HOTPLUG_API
void AsyncPluginImpl::HotPlugEvent(struct libusb_device *usb_device,
                                   libusb_hotplug_event event) {
  ola::thread::MutexLocker locker(&m_mutex);
  if (m_suppress_hotplug_events) {
    return;
  }

  OLA_INFO << "Got USB hotplug event  for " << usb_device << " : "
           << (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED ? "add" : "del");
  if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
    USBDeviceAdded(usb_device);
  } else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
    USBDeviceRemoved(usb_device);
  }
}
#endif

bool AsyncPluginImpl::NewWidget(AnymauDMX *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "Anyma USB Device",
                        "anyma-" + widget->SerialNumber()));
}

bool AsyncPluginImpl::NewWidget(EurolitePro *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "EurolitePro USB Device",
                        "eurolite-" + widget->SerialNumber()));
}

bool AsyncPluginImpl::NewWidget(class JaRuleWidget *widget) {
  return StartAndRegisterDevice(
      widget,
      new JaRuleDevice(m_plugin, widget, "Ja Rule USB Device", "0"));
}

bool AsyncPluginImpl::NewWidget(ScanlimeFadecandy *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(
          m_plugin, widget,
          "Fadecandy USB Device (" + widget->SerialNumber() + ")",
          "fadecandy-" + widget->SerialNumber()));
}

bool AsyncPluginImpl::NewWidget(Sunlite *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "Sunlite USBDMX2 Device", "usbdmx2"));
}

bool AsyncPluginImpl::NewWidget(VellemanK8062 *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "Velleman USB Device", "velleman"));
}

void AsyncPluginImpl::WidgetRemoved(AnymauDMX *widget) {
  RemoveWidget(widget);
}

void AsyncPluginImpl::WidgetRemoved(EurolitePro *widget) {
  RemoveWidget(widget);
}

void AsyncPluginImpl::WidgetRemoved(JaRuleWidget *widget) {
  RemoveWidget(widget);
}

void AsyncPluginImpl::WidgetRemoved(ScanlimeFadecandy *widget) {
  RemoveWidget(widget);
}

void AsyncPluginImpl::WidgetRemoved(Sunlite *widget) {
  RemoveWidget(widget);
}

void AsyncPluginImpl::WidgetRemoved(VellemanK8062 *widget) {
  RemoveWidget(widget);
}

/**
 * @brief Check if this platform supports hotplug.
 * @returns true if hotplug is supported and enabled on this platform, false
 *   otherwise.
 */
bool AsyncPluginImpl::HotplugSupported() {
#ifdef HAVE_LIBUSB_HOTPLUG_API
  return libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) != 0;
#else
  return false;
#endif
}

/**
 * @brief Signal a new USB device has been added.
 *
 * This can be called from either the libusb thread or the main thread. However
 * only one of those will be active at once, so we can avoid locking.
 */
bool AsyncPluginImpl::USBDeviceAdded(libusb_device *usb_device) {
  struct libusb_device_descriptor descriptor;
  libusb_get_device_descriptor(usb_device, &descriptor);

  OLA_DEBUG << "USB device added, checking for widget support, vendor "
            << strings::ToHex(descriptor.idVendor) << ", product "
            << strings::ToHex(descriptor.idProduct);

  WidgetFactories::iterator iter = m_widget_factories.begin();
  for (; iter != m_widget_factories.end(); ++iter) {
    if ((*iter)->DeviceAdded(&m_widget_observer, usb_device, descriptor)) {
      STLReplacePtr(&m_device_factory_map, usb_device, *iter);
      return true;
    }
  }
  return false;
}

/**
 * @brief Signal a USB device has been removed.
 *
 * This can be called from either the libusb thread or the main thread. However
 * only one of those will be active at once, so we can avoid locking.
 */
void AsyncPluginImpl::USBDeviceRemoved(libusb_device *usb_device) {
  WidgetFactory *factory = STLLookupAndRemovePtr(
      &m_device_factory_map, usb_device);
  if (factory) {
    factory->DeviceRemoved(&m_widget_observer, usb_device);
  }
}

/*
 * @brief Signal widget / device addition.
 * @param widget The widget that was added.
 * @param device The new olad device that uses this new widget.
 *
 * This is run within the main thread.
 */
bool AsyncPluginImpl::StartAndRegisterDevice(Widget *widget, Device *device) {
  if (!device->Start()) {
    delete device;
    return false;
  }

  Device *old_device = STLReplacePtr(&m_widget_device_map, widget, device);
  if (old_device) {
    m_plugin_adaptor->UnregisterDevice(old_device);
    old_device->Stop();
    delete old_device;
  }
  m_plugin_adaptor->RegisterDevice(device);
  return true;
}

/*
 * @brief Signal widget removal.
 * @param widget The widget that was removed.
 *
 * This is run within the main thread.
 */
void AsyncPluginImpl::RemoveWidget(Widget *widget) {
  Device *device = STLLookupAndRemovePtr(&m_widget_device_map, widget);
  if (device) {
    m_plugin_adaptor->UnregisterDevice(device);
    device->Stop();
    delete device;
  }
}

/*
 * If hotplug isn't supported, this is called periodically to checked for
 * USB devices that have been added or removed.
 *
 * This is run within the main thread, since the libusb thread only runs if at
 * least one USB device is used.
 */
bool AsyncPluginImpl::ScanUSBDevices() {
  OLA_INFO << "Scanning USB devices....";
  std::set<USBDeviceID> current_device_ids;

  libusb_device **device_list;
  size_t device_count = libusb_get_device_list(m_context, &device_list);

  OLA_INFO << "Got " << device_count << " devices";
  for (unsigned int i = 0; i < device_count; i++) {
    libusb_device *usb_device = device_list[i];

    USBDeviceID device_id(libusb_get_bus_number(usb_device),
                          libusb_get_device_address(usb_device));

    current_device_ids.insert(device_id);

    if (!STLContains(m_seen_usb_devices, device_id)) {
      OLA_INFO << "  " << usb_device;
      bool claimed = USBDeviceAdded(usb_device);
      STLReplace(&m_seen_usb_devices, device_id, claimed ? usb_device : NULL);
    }
  }
  libusb_free_device_list(device_list, 1);  // unref devices

  USBDeviceIDs::iterator iter = m_seen_usb_devices.begin();
  while (iter != m_seen_usb_devices.end()) {
    if (!STLContains(current_device_ids, iter->first)) {
      if (iter->second) {
        USBDeviceRemoved(iter->second);
      }
      m_seen_usb_devices.erase(iter++);
    } else {
      iter++;
    }
  }
  return true;
}

}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
