/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * UsbProPlugin.cpp
 * The UsbPro plugin for ola
 * Copyright (C) 2006-2010 Simon Newton
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
#include "plugins/usbpro/RobeDevice.h"
#include "plugins/usbpro/RobeWidgetDetector.h"
#include "plugins/usbpro/UsbProDevice.h"
#include "plugins/usbpro/UsbProPlugin.h"


namespace ola {
namespace plugin {
namespace usbpro {

using std::auto_ptr;

const char UsbProPlugin::DEFAULT_DEVICE_DIR[] = "/dev";
const char UsbProPlugin::DEFAULT_PRO_FPS_LIMIT[] = "190";
const char UsbProPlugin::DEVICE_DIR_KEY[] = "device_dir";
const char UsbProPlugin::DEVICE_PREFIX_KEY[] = "device_prefix";
const char UsbProPlugin::LINUX_DEVICE_PREFIX[] = "ttyUSB";
const char UsbProPlugin::MAC_DEVICE_PREFIX[] = "cu.usbserial-";
const char UsbProPlugin::ROBE_DEVICE_NAME[] = "Robe Universal Interface";
const char UsbProPlugin::PLUGIN_NAME[] = "Enttec USB Pro";
const char UsbProPlugin::PLUGIN_PREFIX[] = "usbpro";
const char UsbProPlugin::TRI_USE_RAW_RDM_KEY[] = "tri_use_raw_rdm";
const char UsbProPlugin::USBPRO_DEVICE_NAME[] = "Enttec Usb Pro Device";
const char UsbProPlugin::USB_PRO_FPS_LIMIT_KEY[] = "pro_fps_limit";

UsbProPlugin::UsbProPlugin(PluginAdaptor *plugin_adaptor)
    : Plugin(plugin_adaptor),
      m_detector_thread(this, plugin_adaptor) {
}


/*
 * Return the description for this plugin
 */
string UsbProPlugin::Description() const {
    return
"Enttec USB Pro Plugin\n"
"----------------------------\n"
"\n"
"This plugin supports USB devices that emulate a serial port. This includes\n"
" Enttec DMX USB Pro, the DMXking USB DMX512-A & the DMX-TRI, Dmxter, \n"
" Robe Universe Interface. See\n"
"http://opendmx.net/index.php/USB_Protocol_Extensions for more info.\n"
"\n"
"--- Config file : ola-usbpro.conf ---\n"
"\n"
"device_dir = /dev\n"
"The directory to look for devices in\n"
"\n"
"device_prefix = ttyUSB\n"
"The prefix of filenames to consider as devices, multiple keys are allowed\n"
"\n"
"pro_fps_limit = 190\n"
"The max frames per second to send to a Usb Pro or DMXKing device\n"
"tri_use_raw_rdm = [true|false]\n"
"Bypass RDM handling in the {DMX,RDM}-TRI widgets.\n";
}


/*
 * Called when a device is removed
 */
void UsbProPlugin::DeviceRemoved(UsbDevice *device) {
  vector<UsbDevice*>::iterator iter;

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
void UsbProPlugin::NewWidget(
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
void UsbProPlugin::NewWidget(
    EnttecUsbProWidget *widget,
    const UsbProWidgetInformation &information) {
  string device_name = GetDeviceName(information);
  if (device_name.empty())
    device_name = USBPRO_DEVICE_NAME;

  AddDevice(new UsbProDevice(
      m_plugin_adaptor,
      this,
      device_name,
      widget,
      information.esta_id ? information.esta_id : ENTTEC_ESTA_ID,
      information.device_id ? information.device_id : 0,
      information.serial,
      GetProFrameLimit()));
}


/**
 * Handle a new Dmx-Tri Widget.
 */
void UsbProPlugin::NewWidget(
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
      information.serial));
}


/**
 * Handle a new Dmxter.
 */
void UsbProPlugin::NewWidget(
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
void UsbProPlugin::NewWidget(
    RobeWidget *widget,
    const RobeWidgetInformation&) {
  AddDevice(new RobeDevice(m_plugin_adaptor,
                           this,
                           ROBE_DEVICE_NAME,
                           widget));
}


/*
 * Add a new device to the list
 * @param device the new UsbDevice
 */
void UsbProPlugin::AddDevice(UsbDevice *device) {
  if (!device->Start()) {
    delete device;
    return;
  }

  device->SetOnRemove(NewSingleCallback(this,
                                        &UsbProPlugin::DeviceRemoved,
                                        device));
  m_devices.push_back(device);
  m_plugin_adaptor->RegisterDevice(device);
}


/*
 * Start the plugin
 */
bool UsbProPlugin::StartHook() {
  m_detector_thread.SetDeviceDirectory(
      m_preferences->GetValue(DEVICE_DIR_KEY));
  m_detector_thread.SetDevicePrefixes(
      m_preferences->GetMultipleValue(DEVICE_PREFIX_KEY));
  if (!m_detector_thread.Start()) {
    OLA_FATAL << "Failed to start the widget discovery thread";
    return false;
  }
  return true;
}


/*
 * Stop the plugin
 * @return true on sucess, false on failure
 */
bool UsbProPlugin::StopHook() {
  vector<UsbDevice*>::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter)
    DeleteDevice(*iter);
  m_detector_thread.Join(NULL);
  m_devices.clear();
  return true;
}


/*
 * Default to sensible values
 */
bool UsbProPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  bool save = false;

  vector<string> device_prefixes =
    m_preferences->GetMultipleValue(DEVICE_PREFIX_KEY);
  if (device_prefixes.empty()) {
    m_preferences->SetMultipleValue(DEVICE_PREFIX_KEY, LINUX_DEVICE_PREFIX);
    m_preferences->SetMultipleValue(DEVICE_PREFIX_KEY, MAC_DEVICE_PREFIX);
    save = true;
  }

  save |= m_preferences->SetDefaultValue(DEVICE_DIR_KEY, StringValidator(),
                                         DEFAULT_DEVICE_DIR);

  save |= m_preferences->SetDefaultValue(USB_PRO_FPS_LIMIT_KEY,
                                         IntValidator(0, MAX_PRO_FPS_LIMIT),
                                         DEFAULT_PRO_FPS_LIMIT);

  save |= m_preferences->SetDefaultValue(TRI_USE_RAW_RDM_KEY,
                                         BoolValidator(),
                                         BoolValidator::DISABLED);

  if (save)
    m_preferences->Save();

  device_prefixes = m_preferences->GetMultipleValue(DEVICE_PREFIX_KEY);
  if (device_prefixes.empty())
    return false;
  return true;
}


void UsbProPlugin::DeleteDevice(UsbDevice *device) {
  SerialWidgetInterface *widget = device->GetWidget();
  m_plugin_adaptor->UnregisterDevice(device);
  device->Stop();
  delete device;
  m_detector_thread.FreeWidget(widget);
}


/**
 * Get a nicely formatted device name from the widget information.
 */
string UsbProPlugin::GetDeviceName(
    const UsbProWidgetInformation &information) {
  string device_name = information.manufacturer;
  if (!(information.manufacturer.empty() ||
        information.device.empty()))
    device_name += " - ";
  device_name += information.device;
  return device_name;
}


/*
 * Get the Frames per second limit for a pro device
 */
unsigned int UsbProPlugin::GetProFrameLimit() {
  unsigned int fps_limit;
  if (!StringToInt(m_preferences->GetValue(USB_PRO_FPS_LIMIT_KEY) ,
                   &fps_limit))
    StringToInt(DEFAULT_PRO_FPS_LIMIT, &fps_limit);
  return fps_limit;
}
}  // usbpro
}  // plugin
}  // ola
