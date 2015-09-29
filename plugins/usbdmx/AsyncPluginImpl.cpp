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

using ola::strings::IntToString;
using ola::usb::HotplugAgent;
using ola::usb::JaRuleWidget;
using ola::usb::USBDeviceID;
using std::auto_ptr;

AsyncPluginImpl::AsyncPluginImpl(PluginAdaptor *plugin_adaptor,
                                 Plugin *plugin,
                                 unsigned int debug_level)
    : m_plugin_adaptor(plugin_adaptor),
      m_plugin(plugin),
      m_debug_level(debug_level),
      m_widget_observer(this, plugin_adaptor) {
}

AsyncPluginImpl::~AsyncPluginImpl() {
  STLDeleteElements(&m_widget_factories);
}

bool AsyncPluginImpl::Start() {
  auto_ptr<HotplugAgent> agent(new HotplugAgent(
      NewCallback(this, &AsyncPluginImpl::DeviceEvent),
      m_debug_level));

  if (!agent->Init()) {
    return false;
  }

  m_usb_adaptor = agent->GetUSBAdaptor();

  // Setup the factories.
  m_widget_factories.push_back(new AnymauDMXFactory(m_usb_adaptor));
  m_widget_factories.push_back(
      new EuroliteProFactory(m_usb_adaptor));
  m_widget_factories.push_back(
      new JaRuleFactory(m_plugin_adaptor, m_usb_adaptor));
  m_widget_factories.push_back(
      new ScanlimeFadecandyFactory(m_usb_adaptor));
  m_widget_factories.push_back(new SunliteFactory(m_usb_adaptor));
  m_widget_factories.push_back(new VellemanK8062Factory(m_usb_adaptor));

  // If we're using hotplug, this starts the hotplug thread.
  if (!agent->Start()) {
    STLDeleteElements(&m_widget_factories);
    return false;
  }

  m_agent.reset(agent.release());
  return true;
}

bool AsyncPluginImpl::Stop() {
  if (!m_agent.get()) {
    return true;
  }

  m_agent->HaltNotifications();

  // Now we're free to use m_device_map.
  USBDeviceMap::iterator iter = m_device_map.begin();
  for (; iter != m_device_map.end(); ++iter) {
    if (iter->second->factory) {
      iter->second->factory->DeviceRemoved(this, iter->second->usb_device);
      iter->second->factory = NULL;
    }
  }
  STLDeleteValues(&m_device_map);
  STLDeleteElements(&m_widget_factories);
  m_agent->Stop();
  m_agent.reset();
  return true;
}

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
 * This is run in either the thread calling Start() or a hotplug thread,
 * but not both at once.
 */
void AsyncPluginImpl::DeviceEvent(HotplugAgent::EventType event,
                                  struct libusb_device *device) {
  if (event == HotplugAgent::DEVICE_ADDED) {
    USBDeviceAdded(device);
  } else {
    USBDeviceID device_id = m_usb_adaptor->GetDeviceId(device);
    DeviceState *state = STLFindOrNull(m_device_map, device_id);
    if (state && state->factory) {
      state->factory->DeviceRemoved(&m_widget_observer, state->usb_device);
      state->factory = NULL;
    }
    STLRemoveAndDelete(&m_device_map, device_id);
  }
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
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
