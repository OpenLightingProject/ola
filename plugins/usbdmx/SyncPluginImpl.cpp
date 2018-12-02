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
 * SyncPluginImpl.cpp
 * The synchronous implementation of the USB DMX plugin.
 * Copyright (C) 2006 Simon Newton
 */

#include "plugins/usbdmx/SyncPluginImpl.h"

#include <stdlib.h>
#include <stdio.h>
#include <libusb.h>

#include <string>
#include <utility>
#include <vector>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/stl/STLUtils.h"
#include "ola/strings/Format.h"
#include "olad/PluginAdaptor.h"

#include "plugins/usbdmx/AnymauDMX.h"
#include "plugins/usbdmx/AnymauDMXFactory.h"
#include "plugins/usbdmx/AVLdiyD512.h"
#include "plugins/usbdmx/AVLdiyD512Factory.h"
#include "plugins/usbdmx/DMXCProjectsNodleU1.h"
#include "plugins/usbdmx/DMXCProjectsNodleU1Device.h"
#include "plugins/usbdmx/DMXCProjectsNodleU1Factory.h"
#include "plugins/usbdmx/DMXCreator512Basic.h"
#include "plugins/usbdmx/DMXCreator512BasicFactory.h"
#include "plugins/usbdmx/EurolitePro.h"
#include "plugins/usbdmx/EuroliteProFactory.h"
#include "plugins/usbdmx/ScanlimeFadecandy.h"
#include "plugins/usbdmx/ScanlimeFadecandyFactory.h"
#include "plugins/usbdmx/GenericDevice.h"
#include "plugins/usbdmx/ShowJockeyDMXU1.h"
#include "plugins/usbdmx/ShowJockeyDMXU1Factory.h"
#include "plugins/usbdmx/Sunlite.h"
#include "plugins/usbdmx/SunliteFactory.h"
#include "plugins/usbdmx/VellemanK8062.h"
#include "plugins/usbdmx/VellemanK8062Factory.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::pair;
using std::string;
using std::vector;

SyncPluginImpl::SyncPluginImpl(PluginAdaptor *plugin_adaptor,
                               Plugin *plugin,
                               unsigned int debug_level,
                               Preferences *preferences)
    : m_plugin_adaptor(plugin_adaptor),
      m_plugin(plugin),
      m_debug_level(debug_level),
      m_preferences(preferences),
      m_context(NULL) {
  m_widget_factories.push_back(new AnymauDMXFactory(&m_usb_adaptor));
  m_widget_factories.push_back(new AVLdiyD512Factory(&m_usb_adaptor));
  m_widget_factories.push_back(new DMXCProjectsNodleU1Factory(&m_usb_adaptor,
      m_plugin_adaptor, m_preferences));
  m_widget_factories.push_back(new DMXCreator512BasicFactory(&m_usb_adaptor));
  m_widget_factories.push_back(new EuroliteProFactory(&m_usb_adaptor));
  m_widget_factories.push_back(new ScanlimeFadecandyFactory(&m_usb_adaptor));
  m_widget_factories.push_back(new ShowJockeyDMXU1Factory(&m_usb_adaptor));
  m_widget_factories.push_back(new SunliteFactory(&m_usb_adaptor));
  m_widget_factories.push_back(new VellemanK8062Factory(&m_usb_adaptor));
}

SyncPluginImpl::~SyncPluginImpl() {
  STLDeleteElements(&m_widget_factories);
}

bool SyncPluginImpl::Start() {
  if (libusb_init(&m_context)) {
    OLA_WARN << "Failed to init libusb";
    return false;
  }

  OLA_DEBUG << "libusb debug level set to " << m_debug_level;
#ifdef HAVE_LIBUSB_SET_OPTION
  libusb_set_option(m_context, LIBUSB_OPTION_LOG_LEVEL, m_debug_level);
#else
  libusb_set_debug(m_context, m_debug_level);
#endif  // HAVE_LIBUSB_SET_OPTION

  unsigned int devices_claimed = ScanForDevices();
  if (devices_claimed != m_devices.size()) {
    // This indicates there is firmware loading going on, schedule a callback
    // to check for 'new' devices once the firmware has loaded.

    m_plugin_adaptor->RegisterSingleTimeout(
        3500,
        NewSingleCallback(this, &SyncPluginImpl::ReScanForDevices));
  }
  return true;
}

bool SyncPluginImpl::Stop() {
  WidgetToDeviceMap::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->UnregisterDevice(iter->second);
    iter->second->Stop();
    delete iter->second;
    delete iter->first;
  }
  m_devices.clear();
  m_registered_devices.clear();

  libusb_exit(m_context);

  return true;
}

bool SyncPluginImpl::NewWidget(AnymauDMX *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "Anyma USB Device",
                        "anyma-" + widget->SerialNumber()));
}

bool SyncPluginImpl::NewWidget(AVLdiyD512 *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "AVLdiy USB Device",
                        "avldiy-" + widget->SerialNumber()));
}

bool SyncPluginImpl::NewWidget(DMXCProjectsNodleU1 *widget) {
  return StartAndRegisterDevice(
      widget,
      new DMXCProjectsNodleU1Device(
          m_plugin, widget,
          "DMXControl Projects e.V. Nodle U1 (" + widget->SerialNumber() + ")",
          "nodleu1-" + widget->SerialNumber(),
          m_plugin_adaptor));
}

bool SyncPluginImpl::NewWidget(DMXCreator512Basic *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "DMXCreator 512 Basic USB Device",
                        "dmxcreator512basic-" + widget->SerialNumber()));
}

bool SyncPluginImpl::NewWidget(EurolitePro *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "EurolitePro USB Device",
                        "eurolite-" + widget->SerialNumber()));
}

bool SyncPluginImpl::NewWidget(OLA_UNUSED ola::usb::JaRuleWidget *widget) {
  // This should never happen since there is no Synchronous support for Ja Rule.
  OLA_WARN << "::NewWidget called for a JaRuleWidget";
  return false;
}

bool SyncPluginImpl::NewWidget(ScanlimeFadecandy *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(
          m_plugin, widget,
          "Fadecandy USB Device (" + widget->SerialNumber() + ")",
          "fadecandy-" + widget->SerialNumber()));
}

bool SyncPluginImpl::NewWidget(ShowJockeyDMXU1 *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(
          m_plugin, widget,
          "ShowJockey-DMX-U1 Device (" + widget->SerialNumber() + ")",
          "showjockey-dmx-u1-" + widget->SerialNumber()));
}

bool SyncPluginImpl::NewWidget(Sunlite *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "Sunlite USBDMX2 Device", "usbdmx2"));
}

bool SyncPluginImpl::NewWidget(VellemanK8062 *widget) {
  return StartAndRegisterDevice(
      widget,
      new GenericDevice(m_plugin, widget, "Velleman USB Device", "velleman"));
}

/*
 * @brief Look for USB devices.
 */
unsigned int SyncPluginImpl::ScanForDevices() {
  libusb_device **device_list;
  size_t device_count = libusb_get_device_list(m_context, &device_list);
  unsigned int claimed_device_count = 0;

  for (unsigned int i = 0; i < device_count; i++) {
    if (CheckDevice(device_list[i])) {
      claimed_device_count++;
    }
  }
  libusb_free_device_list(device_list, 1);  // unref devices
  return claimed_device_count;
}

bool SyncPluginImpl::CheckDevice(libusb_device *usb_device) {
  struct libusb_device_descriptor device_descriptor;
  libusb_get_device_descriptor(usb_device, &device_descriptor);

  OLA_DEBUG << "USB device found, checking for widget support, vendor "
            << strings::ToHex(device_descriptor.idVendor) << ", product "
            << strings::ToHex(device_descriptor.idProduct);

  pair<uint8_t, uint8_t> bus_dev_id(libusb_get_bus_number(usb_device),
                                    libusb_get_device_address(usb_device));

  if (STLContains(m_registered_devices, bus_dev_id)) {
    return false;
  }

  WidgetFactories::iterator iter = m_widget_factories.begin();
  for (; iter != m_widget_factories.end(); ++iter) {
    if ((*iter)->DeviceAdded(this, usb_device, device_descriptor)) {
      m_registered_devices.insert(bus_dev_id);
      return true;
    }
  }
  return false;
}

void SyncPluginImpl::ReScanForDevices() {
  ScanForDevices();
}

/*
 * @brief Signal widget / device addition.
 * @param widget The widget that was added.
 * @param device The new olad device that uses this new widget.
 */
bool SyncPluginImpl::StartAndRegisterDevice(WidgetInterface *widget,
                                            Device *device) {
  if (!device->Start()) {
    delete device;
    return false;
  }

  STLReplace(&m_devices, widget, device);
  m_plugin_adaptor->RegisterDevice(device);
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
