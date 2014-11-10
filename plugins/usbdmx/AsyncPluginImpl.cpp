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

#include "plugins/usbdmx/AnymaDeviceManager.h"
#include "plugins/usbdmx/SunliteDeviceManager.h"
#include "plugins/usbdmx/EuroliteProDeviceManager.h"

#include "plugins/usbdmx/VellemanDevice.h"

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
    : m_libusb_adaptor(libusb_adaptor),
      m_context(NULL),
      m_use_hotplug(false),
      m_stopping(false) {
  #ifdef OLA_LIBUSB_HAS_HOTPLUG_API
  m_hotplug_handle = 0;
  #endif

  m_device_managers.push_back(new AnymaDeviceManager(plugin_adaptor, plugin));
  m_device_managers.push_back(new SunliteDeviceManager(plugin_adaptor, plugin));
  m_device_managers.push_back(new EuroliteProDeviceManager(plugin_adaptor,
                                                           plugin));
}

AsyncPluginImpl::~AsyncPluginImpl() {
  STLDeleteElements(&m_device_managers);
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
        NewSingleCallback(this, &AsyncPluginImpl::FindDevices));
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
  DeviceToFactoryMap::iterator iter = m_device_factory_map.begin();
  for (; iter != m_device_factory_map.end(); ++iter) {
    iter->second->DeviceRemoved(iter->first);
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
void AsyncPluginImpl::FindDevices() {
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

  DeviceManagers::iterator iter = m_device_managers.begin();
  for (; iter != m_device_managers.end(); ++iter) {
    if ((*iter)->DeviceAdded(usb_device, device_descriptor)) {
      STLReplacePtr(&m_device_factory_map, usb_device, *iter);
      return;
    }
  }

  // Old style
  /*
  if (device_descriptor.idVendor == 0x10cf &&
      device_descriptor.idProduct == 0x8062) {
    OLA_INFO << "Found a Velleman USB device";
    device = new VellemanDevice(m_plugin, usb_device);
  */
}

void AsyncPluginImpl::DeviceRemoved(libusb_device *usb_device) {
  UsbDeviceManagerInterface *factory = STLLookupAndRemovePtr(
      &m_device_factory_map, usb_device);
  if (factory) {
    factory->DeviceRemoved(usb_device);
  }
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
