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
 * UsbSerialPlugin.cpp
 * The UsbPro plugin for ola
 * Copyright (C) 2006 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <memory>
#include <string>
#include <vector>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"

#include "plugins/usbpro/ArduinoRGBDevice.h"
#include "plugins/usbpro/DmxTriDevice.h"
#include "plugins/usbpro/DmxTriWidget.h"
#include "plugins/usbpro/DmxterDevice.h"
#include "plugins/usbpro/OpenDeckDevice.h"
#include "plugins/usbpro/RobeDevice.h"
#include "plugins/usbpro/RobeWidgetDetector.h"
#include "plugins/usbpro/UltraDMXProDevice.h"
#include "plugins/usbpro/UltraDMXProWidget.h"
#include "plugins/usbpro/UsbProDevice.h"
#include "plugins/usbpro/UsbSerialPlugin.h"
#include "plugins/usbpro/UsbSerialPluginDescription.h"


namespace ola {
namespace plugin {
namespace usbpro {

using std::auto_ptr;
using std::string;
using std::vector;

const char UsbSerialPlugin::DEFAULT_DEVICE_DIR[] = "/dev";
const char UsbSerialPlugin::DEVICE_DIR_KEY[] = "device_dir";
const char UsbSerialPlugin::DEVICE_PREFIX_KEY[] = "device_prefix";
const char UsbSerialPlugin::IGNORED_DEVICES_KEY[] = "ignore_device";
const char UsbSerialPlugin::LINUX_DEVICE_PREFIX[] = "ttyUSB";
const char UsbSerialPlugin::BSD_DEVICE_PREFIX[] = "ttyU";
const char UsbSerialPlugin::MAC_DEVICE_PREFIX[] = "cu.usbserial-";
const char UsbSerialPlugin::PLUGIN_NAME[] = "Serial USB";
const char UsbSerialPlugin::PLUGIN_PREFIX[] = "usbserial";
const char UsbSerialPlugin::OPENDECK_FPS_LIMIT_KEY[] = "opendeck_fps_limit";
const char UsbSerialPlugin::ROBE_DEVICE_NAME[] = "Robe Universal Interface";
const char UsbSerialPlugin::TRI_USE_RAW_RDM_KEY[] = "tri_use_raw_rdm";
const char UsbSerialPlugin::USBPRO_DEVICE_NAME[] = "Enttec Usb Pro Device";
const char UsbSerialPlugin::USB_PRO_FPS_LIMIT_KEY[] = "pro_fps_limit";
const char UsbSerialPlugin::ULTRA_FPS_LIMIT_KEY[] = "ultra_fps_limit";

UsbSerialPlugin::UsbSerialPlugin(PluginAdaptor *plugin_adaptor)
    : Plugin(plugin_adaptor),
      m_detector_thread(this, plugin_adaptor) {
}


/*
 * Return the description for this plugin
 */
string UsbSerialPlugin::Description() const {
    return plugin_description;
}


/*
 * Called when a device is removed
 */
void UsbSerialPlugin::DeviceRemoved(UsbSerialDevice *device) {
  vector<UsbSerialDevice*>::iterator iter;

  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    if (*iter == device) {
      break;
    }
  }

  if (iter == m_devices.end()) {
    OLA_WARN << "Couldn't find the device that was removed";
    return;
  }

  DeleteDevice(device);
  m_devices.erase(iter);
}


/**
 * Handle a new ArduinoWidget.
 */
void UsbSerialPlugin::NewWidget(
    ArduinoWidget *widget,
    const UsbProWidgetInformation &information) {
  AddDevice(new ArduinoRGBDevice(
      m_plugin_adaptor,
      this,
      GetDeviceName(information),
      widget,
      information.esta_id,
      information.device_id,
      information.serial));
}


/**
 * Handle a new Enttec Usb Pro Widget.
 */
void UsbSerialPlugin::NewWidget(
    EnttecUsbProWidget *widget,
    const UsbProWidgetInformation &information) {
  string device_name = GetDeviceName(information);
  if (device_name.empty()) {
    device_name = USBPRO_DEVICE_NAME;
  }

  AddDevice(new UsbProDevice(m_plugin_adaptor, this, device_name, widget,
                             information.serial, information.firmware_version,
                             GetProFrameLimit()));
}


/**
 * Handle a new Dmx-Tri Widget.
 */
void UsbSerialPlugin::NewWidget(
    DmxTriWidget *widget,
    const UsbProWidgetInformation &information) {
  widget->UseRawRDM(
      m_preferences->GetValueAsBool(TRI_USE_RAW_RDM_KEY));
  AddDevice(new DmxTriDevice(
      this,
      GetDeviceName(information),
      widget,
      information.esta_id,
      information.device_id,
      information.serial,
      information.firmware_version));
}


/**
 * Handle a new Dmxter.
 */
void UsbSerialPlugin::NewWidget(
    DmxterWidget *widget,
    const UsbProWidgetInformation &information) {
  AddDevice(new DmxterDevice(
      this,
      GetDeviceName(information),
      widget,
      information.esta_id,
      information.device_id,
      information.serial));
}


/**
 * New Robe Universal Interface.
 */
void UsbSerialPlugin::NewWidget(
    RobeWidget *widget,
    const RobeWidgetInformation&) {
  AddDevice(new RobeDevice(m_plugin_adaptor,
                           this,
                           ROBE_DEVICE_NAME,
                           widget));
}


/**
 * A New Ultra DMX Pro Widget
 */
void UsbSerialPlugin::NewWidget(UltraDMXProWidget *widget,
                                const UsbProWidgetInformation &information) {
  AddDevice(new UltraDMXProDevice(
      m_plugin_adaptor,
      this,
      GetDeviceName(information),
      widget,
      information.esta_id,
      information.device_id,
      information.serial,
      information.firmware_version,
      GetUltraDMXProFrameLimit()));
}

/**
 * A New OpenDeck Widget
 */
void UsbSerialPlugin::NewWidget(OpenDeckWidget *widget,
                                const UsbProWidgetInformation &information) {
  AddDevice(new OpenDeckDevice(
      m_plugin_adaptor,
      this,
      GetDeviceName(information),
      widget,
      information.esta_id,
      information.device_id,
      information.serial,
      information.firmware_version,
      GetOpenDeckFrameLimit()));
}

/*
 * Add a new device to the list
 * @param device the new UsbSerialDevice
 */
void UsbSerialPlugin::AddDevice(UsbSerialDevice *device) {
  if (!device->Start()) {
    delete device;
    return;
  }

  device->SetOnRemove(NewSingleCallback(this,
                                        &UsbSerialPlugin::DeviceRemoved,
                                        device));
  m_devices.push_back(device);
  m_plugin_adaptor->RegisterDevice(device);
}


/*
 * Start the plugin
 */
bool UsbSerialPlugin::StartHook() {
  const vector<string> ignored_devices =
      m_preferences->GetMultipleValue(IGNORED_DEVICES_KEY);
  m_detector_thread.SetIgnoredDevices(ignored_devices);

  m_detector_thread.SetDeviceDirectory(
      m_preferences->GetValue(DEVICE_DIR_KEY));
  m_detector_thread.SetDevicePrefixes(
      m_preferences->GetMultipleValue(DEVICE_PREFIX_KEY));
  if (!m_detector_thread.Start()) {
    OLA_FATAL << "Failed to start the widget discovery thread";
    return false;
  }
  m_detector_thread.WaitUntilRunning();
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool UsbSerialPlugin::StopHook() {
  vector<UsbSerialDevice*>::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    DeleteDevice(*iter);
  }
  m_detector_thread.Join(NULL);
  m_devices.clear();
  return true;
}


/*
 * Default to sensible values
 */
bool UsbSerialPlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }

  bool save = false;

  vector<string> device_prefixes =
    m_preferences->GetMultipleValue(DEVICE_PREFIX_KEY);
  if (device_prefixes.empty()) {
    m_preferences->SetMultipleValue(DEVICE_PREFIX_KEY, LINUX_DEVICE_PREFIX);
    m_preferences->SetMultipleValue(DEVICE_PREFIX_KEY, MAC_DEVICE_PREFIX);
    m_preferences->SetMultipleValue(DEVICE_PREFIX_KEY, BSD_DEVICE_PREFIX);
    save = true;
  }

  save |= m_preferences->SetDefaultValue(DEVICE_DIR_KEY, StringValidator(),
                                         DEFAULT_DEVICE_DIR);

  save |= m_preferences->SetDefaultValue(OPENDECK_FPS_LIMIT_KEY,
                                         UIntValidator(0,
                                             MAX_OPENDECK_FPS_LIMIT),
                                             DEFAULT_OPENDECK_FPS_LIMIT);

  save |= m_preferences->SetDefaultValue(USB_PRO_FPS_LIMIT_KEY,
                                         UIntValidator(0, MAX_PRO_FPS_LIMIT),
                                         DEFAULT_PRO_FPS_LIMIT);

  save |= m_preferences->SetDefaultValue(ULTRA_FPS_LIMIT_KEY,
                                         UIntValidator(0, MAX_ULTRA_FPS_LIMIT),
                                         DEFAULT_ULTRA_FPS_LIMIT);

  save |= m_preferences->SetDefaultValue(TRI_USE_RAW_RDM_KEY,
                                         BoolValidator(),
                                         false);

  if (save) {
    m_preferences->Save();
  }

  device_prefixes = m_preferences->GetMultipleValue(DEVICE_PREFIX_KEY);
  if (device_prefixes.empty()) {
    return false;
  }
  return true;
}


void UsbSerialPlugin::DeleteDevice(UsbSerialDevice *device) {
  SerialWidgetInterface *widget = device->GetWidget();
  m_plugin_adaptor->UnregisterDevice(device);
  device->Stop();
  delete device;
  m_detector_thread.FreeWidget(widget);
}


/**
 * Get a nicely formatted device name from the widget information.
 */
string UsbSerialPlugin::GetDeviceName(
    const UsbProWidgetInformation &information) {
  string device_name = information.manufacturer;
  if (!(information.manufacturer.empty() ||
        information.device.empty())) {
    device_name += " - ";
  }
  device_name += information.device;
  return device_name;
}

/*
 * Get the Frames per second limit for OpenDeck device
 */
unsigned int UsbSerialPlugin::GetOpenDeckFrameLimit() {
  unsigned int fps_limit;
  if (!StringToInt(m_preferences->GetValue(OPENDECK_FPS_LIMIT_KEY) ,
                   &fps_limit)) {
    return DEFAULT_OPENDECK_FPS_LIMIT;
  }
  return fps_limit;
}


/*
 * Get the Frames per second limit for a pro device
 */
unsigned int UsbSerialPlugin::GetProFrameLimit() {
  unsigned int fps_limit;
  if (!StringToInt(m_preferences->GetValue(USB_PRO_FPS_LIMIT_KEY) ,
                   &fps_limit)) {
    return DEFAULT_PRO_FPS_LIMIT;
  }
  return fps_limit;
}


/*
 * Get the Frames per second limit for a Ultra DMX Pro Device
 */
unsigned int UsbSerialPlugin::GetUltraDMXProFrameLimit() {
  unsigned int fps_limit;
  if (!StringToInt(m_preferences->GetValue(ULTRA_FPS_LIMIT_KEY) ,
                   &fps_limit)) {
    return DEFAULT_ULTRA_FPS_LIMIT;
  }
  return fps_limit;
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
