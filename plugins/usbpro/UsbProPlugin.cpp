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

#include "ola/BaseTypes.h"
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"

#include "plugins/usbpro/ArduinoRGBDevice.h"
#include "plugins/usbpro/DmxTriDevice.h"
#include "plugins/usbpro/DmxterDevice.h"
#include "plugins/usbpro/RobeDevice.h"
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
      m_detector_thread(
          ola::NewCallback(this, &UsbProPlugin::NewUsbProWidget),
          ola::NewCallback(this, &UsbProPlugin::NewRobeWidget)) {
}


/*
 * Return the description for this plugin
 */
string UsbProPlugin::Description() const {
    return
"Enttec USB Pro Plugin\n"
"----------------------------\n"
"\n"
"This plugin supports devices that implement the Enttec USB Pro specfication\n"
"including the DMX USB Pro, the DMXking USB DMX512-A & the DMX-TRI. See\n"
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


/*
 * Called when a new usb pro widget is detected by the discovery thread.
 * @param widget a pointer to a UsbWidget whose ownership is transferred to us.
 * @param information A WidgetInformation struct for this widget
 */
void UsbProPlugin::NewUsbProWidget(
    BaseUsbProWidget *widget,
    const UsbProWidgetInformation *information_ptr) {
  m_plugin_adaptor->Execute(
      ola::NewSingleCallback(this,
                             &UsbProPlugin::InternalNewUsbProWidget,
                             widget,
                             information_ptr));
}


/*
 * Called when a new robe widget is detected by the discovery thread.
 * @param widget a pointer to a UsbWidget whose ownership is transferred to us.
 * @param information A WidgetInformation struct for this widget
 */
void UsbProPlugin::NewRobeWidget(
    RobeWidget *widget,
    const RobeWidgetInformation *information_ptr) {
  m_plugin_adaptor->Execute(
      ola::NewSingleCallback(this,
                             &UsbProPlugin::InternalNewRobeWidget,
                             widget,
                             information_ptr));
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
 * Handle a new Usb Pro Widget. This is called within the main thread
 */
void UsbProPlugin::InternalNewUsbProWidget(
  class BaseUsbProWidget *widget,
  const UsbProWidgetInformation *information_ptr) {
  m_plugin_adaptor->AddReadDescriptor(widget->GetDescriptor());
  auto_ptr<const UsbProWidgetInformation> information(information_ptr);
  string device_name = information->manufactuer;
  if (!(information->manufactuer.empty() ||
        information->device.empty()))
    device_name += " - ";
  device_name += information->device;
  widget->SetMessageHandler(NULL);

  switch (information->esta_id) {
    case DMX_KING_ESTA_ID:
      if (information->device_id == DMX_KING_DEVICE_ID) {
        // DMxKing devices are drop in replacements for a Usb Pro
        AddDevice(new UsbProDevice(
            m_plugin_adaptor,
            this,
            device_name,
            widget,
            information->esta_id,
            information->device_id,
            information->serial,
            GetProFrameLimit()));
        return;
      }
    case GODDARD_ESTA_ID:
      if (information->device_id == GODDARD_DMXTER4_ID ||
          information->device_id == GODDARD_MINI_DMXTER4_ID) {
        AddDevice(new DmxterDevice(
            this,
            device_name,
            widget,
            information->esta_id,
            information->device_id,
            information->serial));
        return;
      }
      break;
    case JESE_ESTA_ID:
      if (information->device_id == JESE_DMX_TRI_ID ||
          information->device_id == JESE_RDM_TRI_ID) {
        AddDevice(new DmxTriDevice(
            m_plugin_adaptor,
            this,
            device_name,
            widget,
            information->esta_id,
            information->device_id,
            information->serial,
            m_preferences->GetValueAsBool(TRI_USE_RAW_RDM_KEY)));
        return;
      }
      break;
    case OPEN_LIGHTING_ESTA_CODE:
      if (information->device_id == OPEN_LIGHTING_RGB_MIXER_ID ||
          information->device_id == OPEN_LIGHTING_PACKETHEADS_ID) {
        AddDevice(new ArduinoRGBDevice(
            m_plugin_adaptor,
            this,
            device_name,
            widget,
            information->esta_id,
            information->device_id,
            information->serial));
        return;
      }
      break;
  }
  OLA_WARN << "Defaulting to a Usb Pro device";
  device_name = USBPRO_DEVICE_NAME;
  AddDevice(
      new UsbProDevice(m_plugin_adaptor,
                       this,
                       device_name,
                       widget,
                       ENTTEC_ESTA_ID,
                       0,  // assume device id is 0
                       information->serial,
                       GetProFrameLimit()));
}


/**
 * Handle a new Robe Widget. This is called within the main thread
 */
void UsbProPlugin::InternalNewRobeWidget(
    RobeWidget *widget,
    const RobeWidgetInformation *information) {
  AddDevice(new RobeDevice(m_plugin_adaptor,
                           this,
                           ROBE_DEVICE_NAME,
                           widget));
  (void) information;
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
