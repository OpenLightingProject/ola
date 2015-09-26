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

#include "libs/usb/JaRuleWidget.h"
#include "libs/usb/LibUsbAdaptor.h"
#include "libs/usb/LibUsbThread.h"

#include "plugins/usbdmx/AnymauDMX.h"
#include "plugins/usbdmx/AnymauDMXFactory.h"
#include "plugins/usbdmx/EuroliteProFactory.h"
#include "plugins/usbdmx/GenericDevice.h"
#include "plugins/usbdmx/JaRuleDevice.h"
#include "plugins/usbdmx/JaRuleFactory.h"
#include "plugins/usbdmx/ScanlimeFadecandy.h"
#include "plugins/usbdmx/ScanlimeFadecandyFactory.h"
#include "plugins/usbdmx/SunliteFactory.h"
#include "plugins/usbdmx/VellemanK8062.h"
#include "plugins/usbdmx/VellemanK8062Factory.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::usb::USBDeviceID;
using ola::usb::JaRuleWidget;
using ola::strings::IntToString;

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
    m_usb_thread.reset(new ola::usb::LibUsbHotplugThread(
          m_context, hotplug_callback, this));
#else
    OLA_FATAL << "Mismatch between m_use_hotplug and "
      " HAVE_LIBUSB_HOTPLUG_API";
    return false;
#endif
  } else {
    m_usb_thread.reset(new ola::usb::LibUsbSimpleThread(m_context));
  }
  m_usb_adaptor.reset(
      new ola::usb::AsyncronousLibUsbAdaptor(m_usb_thread.get()));

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

  USBDeviceMap::iterator iter = m_device_map.begin();
  for (; iter != m_device_map.end(); ++iter) {
    if (iter->second->factory) {
      iter->second->factory->DeviceRemoved(this, iter->second->usb_device);
      iter->second->factory = NULL;
    }
  }
  STLDeleteValues(&m_device_map);

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
    USBDeviceID device_id = m_usb_adaptor->GetDeviceId(usb_device);
    DeviceState *state = STLFindOrNull(m_device_map, device_id);
    if (state && state->factory) {
      state->factory->DeviceRemoved(&m_widget_observer, state->usb_device);
      state->factory = NULL;
    }
    STLRemoveAndDelete(&m_device_map, device_id);
  }
}
#endif

bool AsyncPluginImpl::NewWidget(AnymauDMX *widget) {
  return StartAndRegisterDevice(
      widget->GetDeviceId(),
      new GenericDevice(m_plugin, widget, "Anyma USB Device",
                        "anyma-" + widget->SerialNumber()));
}

bool AsyncPluginImpl::NewWidget(EurolitePro *widget) {
  return StartAndRegisterDevice(
      widget->GetDeviceId(),
      new GenericDevice(m_plugin, widget, "EurolitePro USB Device",
                        "eurolite-" + widget->SerialNumber()));
}

bool AsyncPluginImpl::NewWidget(JaRuleWidget *widget) {
  std::ostringstream str;
  str << widget->ProductString() << " (" << widget->GetUID() << ")";
  return StartAndRegisterDevice(widget->GetDeviceId(),
                                new JaRuleDevice(m_plugin, widget, str.str()));
}

bool AsyncPluginImpl::NewWidget(ScanlimeFadecandy *widget) {
  return StartAndRegisterDevice(
      widget->GetDeviceId(),
      new GenericDevice(
          m_plugin, widget,
          "Fadecandy USB Device (" + widget->SerialNumber() + ")",
          "fadecandy-" + widget->SerialNumber()));
}

bool AsyncPluginImpl::NewWidget(Sunlite *widget) {
  return StartAndRegisterDevice(
      widget->GetDeviceId(),
      new GenericDevice(m_plugin, widget, "Sunlite USBDMX2 Device", "usbdmx2"));
}

bool AsyncPluginImpl::NewWidget(VellemanK8062 *widget) {
  return StartAndRegisterDevice(
      widget->GetDeviceId(),
      new GenericDevice(m_plugin, widget, "Velleman USB Device", "velleman"));
}

void AsyncPluginImpl::WidgetRemoved(AnymauDMX *widget) {
  RemoveWidget(widget->GetDeviceId());
}

void AsyncPluginImpl::WidgetRemoved(EurolitePro *widget) {
  RemoveWidget(widget->GetDeviceId());
}

void AsyncPluginImpl::WidgetRemoved(JaRuleWidget *widget) {
  RemoveWidget(widget->GetDeviceId());
}

void AsyncPluginImpl::WidgetRemoved(ScanlimeFadecandy *widget) {
  RemoveWidget(widget->GetDeviceId());
}

void AsyncPluginImpl::WidgetRemoved(Sunlite *widget) {
  RemoveWidget(widget->GetDeviceId());
}

void AsyncPluginImpl::WidgetRemoved(VellemanK8062 *widget) {
  RemoveWidget(widget->GetDeviceId());
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
 * This can be called more than once for a given device, in this case we'll
 * avoid calling the factories twice.
 *
 * This can be called from either the libusb thread or the main thread. However
 * only one of those will be active at once, so we can avoid locking.
 */
void AsyncPluginImpl::USBDeviceAdded(libusb_device *usb_device) {
  USBDeviceID device_id = m_usb_adaptor->GetDeviceId(usb_device);
  USBDeviceMap::iterator iter = STLLookupOrInsertNew(&m_device_map, device_id);

  DeviceState *state = iter->second;
  if (!state->usb_device) {
    state->usb_device = usb_device;
  }

  if (state->factory) {
    // Already claimed
    return;
  }

  struct libusb_device_descriptor descriptor;
  libusb_get_device_descriptor(usb_device, &descriptor);

  OLA_DEBUG << "USB device added, checking for widget support, vendor "
            << strings::ToHex(descriptor.idVendor) << ", product "
            << strings::ToHex(descriptor.idProduct);

  WidgetFactories::iterator factory_iter = m_widget_factories.begin();
  for (; factory_iter != m_widget_factories.end(); ++factory_iter) {
    if ((*factory_iter)->DeviceAdded(&m_widget_observer, usb_device,
                                     descriptor)) {
      state->factory = *factory_iter;
      return;
    }
  }
}

/*
 * @brief Signal widget / device addition.
 * @param device_id The device id to add.
 * @param device The new olad device that uses this new widget.
 *
 * This is run within the main thread.
# */
bool AsyncPluginImpl::StartAndRegisterDevice(const USBDeviceID &device_id,
                                             Device *device) {
  if (!device->Start()) {
    delete device;
    return false;
  }

  DeviceState *state = STLFindOrNull(m_device_map, device_id);
  if (!state) {
    OLA_WARN << "Failed to find state for device "
             << IntToString(device_id.first) << ":"
             << IntToString(device_id.second);
    delete device;
    return false;
  }

  if (state->ola_device) {
    OLA_WARN << "Clobbering an old device!";
    m_plugin_adaptor->UnregisterDevice(state->ola_device);
    state->ola_device->Stop();
    delete state->ola_device;
  }
  m_plugin_adaptor->RegisterDevice(device);
  state->ola_device = device;
  return true;
}

/*
 * @brief Signal widget removal.
 * @param device_id The device id to remove.
 *
 * This is run within the main thread.
 */
void AsyncPluginImpl::RemoveWidget(const ola::usb::USBDeviceID &device_id) {
  DeviceState *state = STLFindOrNull(m_device_map, device_id);
  if (state && state->ola_device) {
    m_plugin_adaptor->UnregisterDevice(state->ola_device);
    state->ola_device->Stop();
    delete state->ola_device;
    state->ola_device = NULL;
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

    OLA_INFO << "  " << usb_device;
    USBDeviceAdded(usb_device);
  }
  libusb_free_device_list(device_list, 1);  // unref devices

  USBDeviceMap::iterator iter = m_device_map.begin();
  while (iter != m_device_map.end()) {
    if (!STLContains(current_device_ids, iter->first)) {
      DeviceState *state = iter->second;
      if (state && state->factory) {
        state->factory->DeviceRemoved(this, iter->second->usb_device);
        state->factory = NULL;
      }
      delete iter->second;
      m_device_map.erase(iter++);
    } else {
      iter++;
    }
  }
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
