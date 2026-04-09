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
#include "ola/strings/Format.h"
#include "ola/thread/Future.h"
#include "ola/util/Deleter.h"
#include "olad/PluginAdaptor.h"

#include "libs/usb/JaRuleWidget.h"
#include "libs/usb/LibUsbAdaptor.h"
#include "libs/usb/LibUsbThread.h"
#include "libs/usb/Types.h"

#include "plugins/usbdmx/AnymauDMX.h"
#include "plugins/usbdmx/AnymauDMXFactory.h"
#include "plugins/usbdmx/AVLdiyD512.h"
#include "plugins/usbdmx/AVLdiyD512Factory.h"
#include "plugins/usbdmx/DMXCProjectsNodleU1.h"
#include "plugins/usbdmx/DMXCProjectsNodleU1Device.h"
#include "plugins/usbdmx/DMXCProjectsNodleU1Factory.h"
#include "plugins/usbdmx/DMXCreator512Basic.h"
#include "plugins/usbdmx/DMXCreator512BasicFactory.h"
#include "plugins/usbdmx/EuroliteProFactory.h"
#include "plugins/usbdmx/GenericDevice.h"
#include "plugins/usbdmx/JaRuleDevice.h"
#include "plugins/usbdmx/JaRuleFactory.h"
#include "plugins/usbdmx/ScanlimeFadecandy.h"
#include "plugins/usbdmx/ScanlimeFadecandyFactory.h"
#include "plugins/usbdmx/ShowJockeyDMXU1Factory.h"
#include "plugins/usbdmx/SunliteFactory.h"
#include "plugins/usbdmx/VellemanK8062.h"
#include "plugins/usbdmx/VellemanK8062Factory.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::usb::HotplugAgent;
using ola::usb::JaRuleWidget;
using ola::usb::USBDeviceID;
using ola::NewSingleCallback;
using std::auto_ptr;
using ola::thread::Future;

class DeviceState {
 public:
  typedef ola::SingleUseCallback0<void> DeleterCallback;

  DeviceState() : factory(NULL), ola_device(NULL), m_deleter(NULL) {}

  void SetDeleteCallback(DeleterCallback *cb) {
    m_deleter = cb;
  }

  void DeleteWidget() {
    if (m_deleter) {
      m_deleter->Run();
      m_deleter = NULL;
    }
  }

  WidgetFactory *factory;  // The factory that owns this device.
  Device *ola_device;  // The OLA device that uses this USB device.

 private:
  DeleterCallback *m_deleter;
};

AsyncPluginImpl::AsyncPluginImpl(PluginAdaptor *plugin_adaptor,
                                 Plugin *plugin,
                                 unsigned int debug_level,
                                 Preferences *preferences)
    : m_plugin_adaptor(plugin_adaptor),
      m_plugin(plugin),
      m_debug_level(debug_level),
      m_preferences(preferences),
      m_widget_observer(this, plugin_adaptor),
      m_usb_adaptor(NULL) {
}

AsyncPluginImpl::~AsyncPluginImpl() {
  STLDeleteElements(&m_widget_factories);
}

bool AsyncPluginImpl::Start() {
  auto_ptr<HotplugAgent> agent(new HotplugAgent(
      NewCallback(this, &AsyncPluginImpl::DeviceEvent), m_debug_level));

  if (!agent->Init()) {
    return false;
  }

  m_usb_adaptor = agent->GetUSBAdaptor();

  // Setup the factories.
  m_widget_factories.push_back(new AnymauDMXFactory(m_usb_adaptor));
  m_widget_factories.push_back(new AVLdiyD512Factory(m_usb_adaptor));
  m_widget_factories.push_back(
      new DMXCProjectsNodleU1Factory(m_usb_adaptor, m_plugin_adaptor,
                                     m_preferences));
  m_widget_factories.push_back(new DMXCreator512BasicFactory(m_usb_adaptor));
  m_widget_factories.push_back(
      new EuroliteProFactory(m_usb_adaptor, m_preferences));
  m_widget_factories.push_back(
      new JaRuleFactory(m_plugin_adaptor, m_usb_adaptor));
  m_widget_factories.push_back(
      new ScanlimeFadecandyFactory(m_usb_adaptor));
  m_widget_factories.push_back(new ShowJockeyDMXU1Factory(m_usb_adaptor));
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
    DeviceState *state = iter->second;
    if (state->ola_device) {
      m_plugin_adaptor->UnregisterDevice(state->ola_device);
      state->ola_device->Stop();
      delete state->ola_device;
    }
    state->DeleteWidget();
  }
  STLDeleteValues(&m_device_map);
  STLDeleteElements(&m_widget_factories);
  m_agent->Stop();
  m_agent.reset();
  return true;
}

bool AsyncPluginImpl::NewWidget(AnymauDMX *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "Anyma USB Device",
                        "anyma-" + widget->SerialNumber()));
}

bool AsyncPluginImpl::NewWidget(AVLdiyD512 *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "AVLdiy USB Device",
                        "avldiy-" + widget->SerialNumber()));
}

bool AsyncPluginImpl::NewWidget(DMXCProjectsNodleU1 *widget) {
  return StartAndRegisterDevice(
      widget,
      new DMXCProjectsNodleU1Device(
          m_plugin, widget,
          "DMXControl Projects e.V. Nodle U1 (" + widget->SerialNumber() + ")",
          "nodleu1-" + widget->SerialNumber(),
          m_plugin_adaptor));
}

bool AsyncPluginImpl::NewWidget(DMXCreator512Basic *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "DMXCreator 512 Basic USB Device",
                        "dmxcreator512basic-" + widget->SerialNumber()));
}

bool AsyncPluginImpl::NewWidget(EurolitePro *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "EurolitePro USB Device",
                        "eurolite-" + widget->SerialNumber()));
}

bool AsyncPluginImpl::NewWidget(JaRuleWidget *widget) {
  std::ostringstream str;
  str << widget->ProductString() << " (" << widget->GetUID() << ")";
  return StartAndRegisterDevice(widget,
                                new JaRuleDevice(m_plugin, widget, str.str()));
}

bool AsyncPluginImpl::NewWidget(ScanlimeFadecandy *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(
          m_plugin, widget,
          "Fadecandy USB Device (" + widget->SerialNumber() + ")",
          "fadecandy-" + widget->SerialNumber()));
}

bool AsyncPluginImpl::NewWidget(ShowJockeyDMXU1 *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(
          m_plugin, widget,
          "ShowJockey-DMX-U1 Device (" + widget->SerialNumber() + ")",
          "showjockey-dmx-u1-" + widget->SerialNumber()));
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

/**
 * This is run in either the thread calling Start() or a hotplug thread,
 * but not both at once.
 */
void AsyncPluginImpl::DeviceEvent(HotplugAgent::EventType event,
                                  struct libusb_device *device) {
  if (event == HotplugAgent::DEVICE_ADDED) {
    SetupUSBDevice(device);
  } else {
    USBDeviceID device_id = m_usb_adaptor->GetDeviceId(device);
    DeviceState *state;
    if (!STLLookupAndRemove(&m_device_map, device_id, &state) || !state) {
      return;
    }

    // At some point we'll need to notify the factory here. For instance in the
    // Sunlite plugin, if we make the f/w load async we'll need to let the
    // factory cancel the load.

    // Unregister & delete the device and widget in the main thread.
    m_plugin_adaptor->Execute(
        NewSingleCallback(this, &AsyncPluginImpl::ShutdownDeviceState, state));
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
void AsyncPluginImpl::SetupUSBDevice(libusb_device *usb_device) {
  USBDeviceID device_id = m_usb_adaptor->GetDeviceId(usb_device);
  USBDeviceMap::iterator iter = STLLookupOrInsertNew(&m_device_map, device_id);

  DeviceState *state = iter->second;

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
      OLA_INFO << "Device " << device_id << " claimed by "
               << (*factory_iter)->Name();
      state->factory = *factory_iter;
      return;
    }
  }
}

/*
 * @brief Called when a new OLA device is ready.
 * @param widget The Widget used for this device.
 * @param device The new olad device that uses this new widget.
 *
 * This is run within the main thread.
 */
template <typename Widget>
bool AsyncPluginImpl::StartAndRegisterDevice(Widget *widget, Device *device) {
  DeviceState *state = STLFindOrNull(m_device_map, widget->GetDeviceId());
  if (!state) {
    OLA_WARN << "Failed to find state for device " << widget->GetDeviceId();
    delete device;
    return false;
  }

  if (state->ola_device) {
    OLA_WARN << "Clobbering an old device!";
    m_plugin_adaptor->UnregisterDevice(state->ola_device);
    state->ola_device->Stop();
    delete state->ola_device;
    state->ola_device = NULL;
  }

  if (!device->Start()) {
    delete device;
    return false;
  }

  m_plugin_adaptor->RegisterDevice(device);
  state->ola_device = device;
  state->SetDeleteCallback(ola::DeletePointerCallback(widget));
  return true;
}

/*
 * @brief Signal widget removal.
 * @param state The DeviceState owning the widget to be removed.
 *
 * This must be run within the main thread. The Device destructor below may
 * cause libusb_close() to be called, which would deadlock if the hotplug
 * event thread were to wait for it, say in AsyncPluginImpl::DeviceEvent().
 */
void AsyncPluginImpl::ShutdownDeviceState(DeviceState *state) {
  if (state->ola_device) {
    Device *device = state->ola_device;
    m_plugin_adaptor->UnregisterDevice(device);
    device->Stop();
    delete device;
    state->ola_device = NULL;
  } else {
    /* This case can be legitimate when the widget setup through the
     * widget factory is delayed and an unplug event happens before
     * that is completed. */
    OLA_DEBUG << "ola_device was NULL at shutdown";
  }
  state->DeleteWidget();
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
