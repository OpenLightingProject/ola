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
 * UsbSerialPlugin.cpp
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
#include "plugins/usbpro/UltraDMXProDevice.h"
#include "plugins/usbpro/UltraDMXProWidget.h"
#include "plugins/usbpro/UsbProDevice.h"
#include "plugins/usbpro/UsbSerialPlugin.h"


namespace ola {
namespace plugin {
namespace usbpro {

using std::auto_ptr;

const char UsbSerialPlugin::DEFAULT_DEVICE_DIR[] = "/dev";
const char UsbSerialPlugin::DEFAULT_DMX_TRI_FPS_LIMIT[] = "40";
const char UsbSerialPlugin::DEFAULT_PRO_FPS_LIMIT[] = "190";
const char UsbSerialPlugin::DEFAULT_ULTRA_FPS_LIMIT[] = "40";
const char UsbSerialPlugin::DEVICE_DIR_KEY[] = "device_dir";
const char UsbSerialPlugin::DEVICE_PREFIX_KEY[] = "device_prefix";
const char UsbSerialPlugin::DMX_TRI_FPS_LIMIT_KEY[] = "dmx_tri_fps_limit";
const char UsbSerialPlugin::LINUX_DEVICE_PREFIX[] = "ttyUSB";
const char UsbSerialPlugin::MAC_DEVICE_PREFIX[] = "cu.usbserial-";
const char UsbSerialPlugin::PLUGIN_NAME[] = "Serial USB";
const char UsbSerialPlugin::PLUGIN_PREFIX[] = "usbserial";
const char UsbSerialPlugin::ROBE_DEVICE_NAME[] = "Robe Universal Interface";
const char UsbSerialPlugin::TRI_USE_RAW_RDM_KEY[] = "tri_use_raw_rdm";
const char UsbSerialPlugin::USBPRO_DEVICE_NAME[] = "Enttec Usb Pro Device";
const char UsbSerialPlugin::USB_PRO_FPS_LIMIT_KEY[] = "pro_fps_limit";
const char UsbSerialPlugin::ULTRA_FPS_LIMIT_KEY[] = "ultra_fps_limit";
const uint16_t UsbSerialPlugin::ENTTEC_ESTA_ID = 0x454E;

UsbSerialPlugin::UsbSerialPlugin(PluginAdaptor *plugin_adaptor)
    : Plugin(plugin_adaptor),
      m_detector_thread(this, plugin_adaptor) {
}


/*
 * Return the description for this plugin
 */
string UsbSerialPlugin::Description() const {
    return
"Serial USB Plugin\n"
"----------------------------\n"
"\n"
"This plugin supports DMX USB devices that emulate a serial port. This \n"
"includes:\n"
" - Arduino RGB Mixer\n"
" - DMX-TRI & RDM-TRI\n"
" - DMXking USB DMX512-A, Ultra DMX, Ultra DMX Pro\n"
" - DMXter4 & mini DMXter\n"
" - Enttec DMX USB Pro\n"
" - Robe Universe Interface\n"
"\n"
"See http://opendmx.net/index.php/USB_Protocol_Extensions for more info.\n"
"\n"
"--- Config file : ola-usbserial.conf ---\n"
"\n"
"device_dir = /dev\n"
"The directory to look for devices in\n"
"\n"
"device_prefix = ttyUSB\n"
"The prefix of filenames to consider as devices, multiple keys are allowed\n"
"\n"
"dmx_tri_fps_limit = 190\n"
"The max frames per second to send to a DMX-TRI or RDM-TRI device\n"
"\n"
"pro_fps_limit = 190\n"
"The max frames per second to send to a Usb Pro or DMXKing device\n"
"\n"
"tri_use_raw_rdm = [true|false]\n"
"Bypass RDM handling in the {DMX,RDM}-TRI widgets.\n"
"\n"
"ultra_fps_limit = 40\n"
"The max frames per second to send to a Ultra DMX Pro device\n"
"\n";
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
void UsbSerialPlugin::NewWidget(
    DmxTriWidget *widget,
    const UsbProWidgetInformation &information) {
  widget->UseRawRDM(
      m_preferences->GetValueAsBool(TRI_USE_RAW_RDM_KEY));
  AddDevice(new DmxTriDevice(
      m_plugin_adaptor,
      this,
      GetDeviceName(information),
      widget,
      information.esta_id,
      information.device_id,
      information.serial,
      GetDmxTriFrameLimit()));
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
      GetUltraDMXProFrameLimit()));
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
 * @return true on sucess, false on failure
 */
bool UsbSerialPlugin::StopHook() {
  vector<UsbSerialDevice*>::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter)
    DeleteDevice(*iter);
  m_detector_thread.Join(NULL);
  m_devices.clear();
  return true;
}


/*
 * Default to sensible values
 */
bool UsbSerialPlugin::SetDefaultPreferences() {
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

  save |= m_preferences->SetDefaultValue(
      DMX_TRI_FPS_LIMIT_KEY,
      IntValidator(0, MAX_DMX_TRI_FPS_LIMIT),
      DEFAULT_DMX_TRI_FPS_LIMIT);

  save |= m_preferences->SetDefaultValue(USB_PRO_FPS_LIMIT_KEY,
                                         IntValidator(0, MAX_PRO_FPS_LIMIT),
                                         DEFAULT_PRO_FPS_LIMIT);

  save |= m_preferences->SetDefaultValue(ULTRA_FPS_LIMIT_KEY,
                                         IntValidator(0, MAX_ULTRA_FPS_LIMIT),
                                         DEFAULT_ULTRA_FPS_LIMIT);

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
        information.device.empty()))
    device_name += " - ";
  device_name += information.device;
  return device_name;
}


/*
 * Get the Frames per second limit for a pro device
 */
unsigned int UsbSerialPlugin::GetProFrameLimit() {
  unsigned int fps_limit;
  if (!StringToInt(m_preferences->GetValue(USB_PRO_FPS_LIMIT_KEY) ,
                   &fps_limit))
    StringToInt(DEFAULT_PRO_FPS_LIMIT, &fps_limit);
  return fps_limit;
}


/*
 * Get the Frames per second limit for a tri device
 */
unsigned int UsbSerialPlugin::GetDmxTriFrameLimit() {
  unsigned int fps_limit;
  if (!StringToInt(m_preferences->GetValue(DMX_TRI_FPS_LIMIT_KEY) ,
                   &fps_limit))
    StringToInt(DEFAULT_DMX_TRI_FPS_LIMIT, &fps_limit);
  return fps_limit;
}


/*
 * Get the Frames per second limit for a Ultra DMX Pro Device
 */
unsigned int UsbSerialPlugin::GetUltraDMXProFrameLimit() {
  unsigned int fps_limit;
  if (!StringToInt(m_preferences->GetValue(ULTRA_FPS_LIMIT_KEY) ,
                   &fps_limit))
    StringToInt(DEFAULT_ULTRA_FPS_LIMIT, &fps_limit);
  return fps_limit;
}
}  // usbpro
}  // plugin
}  // ola
