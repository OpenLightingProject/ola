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

#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"
#include "olad/PluginAdaptor.h"

#include "plugins/usbdmx/AnymaWidget.h"
#include "plugins/usbdmx/AnymaWidgetFactory.h"
#include "plugins/usbdmx/EuroliteProWidgetFactory.h"
#include "plugins/usbdmx/GenericDevice.h"
#include "plugins/usbdmx/SunliteWidgetFactory.h"
#include "plugins/usbdmx/VellemanWidget.h"
#include "plugins/usbdmx/VellemanWidgetFactory.h"

namespace ola {
namespace plugin {
namespace usbdmx {

namespace {

/*
 * Called by libusb when a USB device is added / removed.
 */
int hotplug_callback(OLA_UNUSED struct libusb_context *ctx,
                     struct libusb_device *dev,
                     libusb_hotplug_event event,
                     void *user_data) {
  AsyncPluginImpl *plugin = reinterpret_cast<AsyncPluginImpl*>(user_data);
  plugin->HotPlugEvent(dev, event);
  return 0;
}
}  // namespace

class LibUsbThread : private ola::thread::Thread {
 public:
  explicit LibUsbThread(libusb_context *context)
    : m_context(context),
      m_term(false),
      m_device_count(0) {
  }

  #ifdef OLA_LIBUSB_HAS_HOTPLUG_API
  void HotPlugStart();
  void HotPlugStop(libusb_hotplug_callback_handle handle);
  #endif

  void AddDevice();
  void RemoveDevice(libusb_device_handle *handle);

  void *Run();

 private:
  libusb_context *m_context;
  bool m_term;  // GUARDED_BY(m_term_mutex)
  ola::thread::Mutex m_term_mutex;
  unsigned int m_device_count;
};

#ifdef OLA_LIBUSB_HAS_HOTPLUG_API
void LibUsbThread::HotPlugStart() {
  OLA_INFO << "STarting libusb thread";
  Start();
}

void LibUsbThread::HotPlugStop(libusb_hotplug_callback_handle handle) {
  OLA_INFO << "stopping libusb thread";
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  libusb_hotplug_deregister_callback(m_context, handle);
  Join();
}
#endif

void LibUsbThread::AddDevice() {
  m_device_count++;
  if (m_device_count == 1) {
    Start();
  }
}

void LibUsbThread::RemoveDevice(libusb_device_handle *handle) {
  if (m_device_count == 1) {
    {
      ola::thread::MutexLocker locker(&m_term_mutex);
      m_term = true;
    }
  }
  libusb_close(handle);
  if (m_device_count == 1) {
    Join();
  }
  m_device_count--;
}

void *LibUsbThread::Run() {
  OLA_INFO << "----libusb event thread is running";
  while (1) {
    {
      ola::thread::MutexLocker locker(&m_term_mutex);
      if (m_term)
        break;
    }
    // TODO(simon): If hotplug isn't active, use libusb_handle_events_timeout
    // here.
    libusb_handle_events(m_context);
  }
  OLA_INFO << "----libusb thread exiting";
  return NULL;
}

AsyncPluginImpl::AsyncPluginImpl(PluginAdaptor *plugin_adaptor,
                                 Plugin *plugin,
                                 LibUsbAdaptor *libusb_adaptor)
    : m_plugin_adaptor(plugin_adaptor),
      m_plugin(plugin),
      m_libusb_adaptor(libusb_adaptor),
      m_context(NULL),
      m_use_hotplug(false),
      m_stopping(false) {
  #ifdef OLA_LIBUSB_HAS_HOTPLUG_API
  m_hotplug_handle = 0;
  #endif

  m_widget_factories.push_back(new AnymaWidgetFactory());
  m_widget_factories.push_back(new EuroliteProWidgetFactory());
  m_widget_factories.push_back(new SunliteWidgetFactory());
  m_widget_factories.push_back(new VellemanWidgetFactory());
}

AsyncPluginImpl::~AsyncPluginImpl() {
  STLDeleteElements(&m_widget_factories);
}

bool AsyncPluginImpl::Start() {
  if (libusb_init(&m_context)) {
    OLA_WARN << "Failed to init libusb";
    return false;
  }

  m_libusb_adaptor->SetDebug(m_context);

  m_use_hotplug = SetupHotPlug();
  OLA_INFO << "SetupHotPlug returned " << m_use_hotplug;
  if (m_use_hotplug) {
    m_usb_thread.reset((new LibUsbThread(m_context)));
    m_usb_thread->HotPlugStart();
  } else {
    // Either we don't support hotplug or the setup failed.
    // As poor man's hotplug, we call libusb_get_device_list periodically to
    // check for new devices.

    /*
    m_plugin_adaptor->RegisterRepeatingTimeout(
        3500,
        NewSingleCallback(this, &AsyncPluginImpl::FindUSBDevices));
    */
  }

  return true;
}

bool AsyncPluginImpl::Stop() {
  m_stopping = true;

  if (m_usb_thread.get()) {
    if (m_use_hotplug) {
      m_usb_thread->HotPlugStop(m_hotplug_handle);
    }
  }
  m_usb_thread.reset();

  // I think we need a lock here
  USBDeviceToFactoryMap::iterator iter = m_device_factory_map.begin();
  for (; iter != m_device_factory_map.end(); ++iter) {
    iter->second->DeviceRemoved(this, iter->first);
  }
  m_device_factory_map.clear();

  libusb_exit(m_context);
  m_context = NULL;
  return true;
}


#ifdef OLA_LIBUSB_HAS_HOTPLUG_API
void AsyncPluginImpl::HotPlugEvent(struct libusb_device *usb_device,
                                   libusb_hotplug_event event) {
  OLA_INFO << "Got USB hotplug event  for " << usb_device << " : "
           << (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED ? "add" : "del");
  if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
    DeviceAdded(usb_device);
  } else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
    DeviceRemoved(usb_device);
  }
}
#endif

bool AsyncPluginImpl::NewWidget(class AnymaWidget *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "Anyma USB Device",
                        "anyma-" + widget->SerialNumber()));
}

bool AsyncPluginImpl::NewWidget(class EuroliteProWidget *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "EurolitePro USB Device",
                        "eurolite-" + widget->SerialNumber()));
}

bool AsyncPluginImpl::NewWidget(class SunliteWidget *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "Sunlite USBDMX2 Device", "usbdmx2"));
}

bool AsyncPluginImpl::NewWidget(class VellemanWidget *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "Velleman USB Device", "velleman"));
}

void AsyncPluginImpl::WidgetRemoved(class AnymaWidget *widget) {
  RemoveWidget(widget);
}

void AsyncPluginImpl::WidgetRemoved(class EuroliteProWidget *widget) {
  RemoveWidget(widget);
}

void AsyncPluginImpl::WidgetRemoved(class SunliteWidget *widget) {
  RemoveWidget(widget);
}

void AsyncPluginImpl::WidgetRemoved(class VellemanWidget *widget) {
  RemoveWidget(widget);
}

bool AsyncPluginImpl::SetupHotPlug() {
#if defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01000102)
  if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) == 0) {
    return false;
  }

  OLA_INFO << "Calling libusb_hotplug_register_callback";
  int rc = libusb_hotplug_register_callback(
      NULL,
      static_cast<libusb_hotplug_event>(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                                        LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
      LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY,
      LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
      hotplug_callback, this, &m_hotplug_handle);

  if (LIBUSB_SUCCESS != rc) {
    OLA_WARN << "Error creating a hotplug callback";
    return false;
  }
  OLA_INFO << "libusb_hotplug_register_callback passed";
  return true;
#else
  return false;
#endif
}

/*
 * Find known devices & register them
 */
void AsyncPluginImpl::FindUSBDevices() {
  libusb_device **device_list;
  size_t device_count = libusb_get_device_list(NULL, &device_list);

  for (unsigned int i = 0; i < device_count; i++) {
    DeviceAdded(device_list[i]);
  }
  libusb_free_device_list(device_list, 1);  // unref devices
}

void AsyncPluginImpl::DeviceAdded(libusb_device *usb_device) {
  struct libusb_device_descriptor device_descriptor;
  libusb_get_device_descriptor(usb_device, &device_descriptor);

  WidgetFactories::iterator iter = m_widget_factories.begin();
  for (; iter != m_widget_factories.end(); ++iter) {
    if ((*iter)->DeviceAdded(this, usb_device, device_descriptor)) {
      STLReplacePtr(&m_device_factory_map, usb_device, *iter);
      return;
    }
  }
}

void AsyncPluginImpl::DeviceRemoved(libusb_device *usb_device) {
  WidgetFactory *factory = STLLookupAndRemovePtr(
      &m_device_factory_map, usb_device);
  if (factory) {
    factory->DeviceRemoved(this, usb_device);
  }
}

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

void AsyncPluginImpl::RemoveWidget(class Widget *widget) {
  Device *device = STLLookupAndRemovePtr(&m_widget_device_map, widget);
  if (device) {
    m_plugin_adaptor->UnregisterDevice(device);
    device->Stop();
    delete device;
  }
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
