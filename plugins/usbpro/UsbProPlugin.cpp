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

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "ola/Closure.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"

#include "plugins/usbpro/UsbProPlugin.h"
#include "plugins/usbpro/ArduinoRGBDevice.h"
#include "plugins/usbpro/DmxTriDevice.h"
#include "plugins/usbpro/UsbProDevice.h"


namespace ola {
namespace plugin {
namespace usbpro {

const char UsbProPlugin::DEFAULT_DEVICE_DIR[] = "/dev";
const char UsbProPlugin::DEFAULT_PRO_FPS_LIMIT[] = "190";
const char UsbProPlugin::DEVICE_DIR_KEY[] = "device_dir";
const char UsbProPlugin::DEVICE_PREFIX_KEY[] = "device_prefix";
const char UsbProPlugin::LINUX_DEVICE_PREFIX[] = "ttyUSB";
const char UsbProPlugin::MAC_DEVICE_PREFIX[] = "cu.usbserial-";
const char UsbProPlugin::PLUGIN_NAME[] = "UsbPro Plugin";
const char UsbProPlugin::PLUGIN_PREFIX[] = "usbpro";
const char UsbProPlugin::USBPRO_DEVICE_NAME[] = "Enttec Usb Pro Device";
const char UsbProPlugin::USB_PRO_FPS_LIMIT_KEY[] = "pro_fps_limit";


/*
 * Return the description for this plugin
 */
string UsbProPlugin::Description() const {
    return
"Enttec Usb Pro Plugin\n"
"----------------------------\n"
"\n"
"This plugin supports devices that implement the Usb Pro spec. See\n"
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
"The max frames per second to send to a Usb Pro or DMXKing device\n";
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
 * Called when a new widget is detected
 * @param widget a pointer to a UsbWidget whose ownership is transferred to us.
 * @param information A DeviceInformation struct for this widget
 */
void UsbProPlugin::NewWidget(class UsbWidget *widget,
                             const DeviceInformation &information) {
  string device_name = information.manufactuer;
  if (!(information.manufactuer.empty() ||
        information.device.empty()))
    device_name += " - ";
  device_name += information.device;
  uint32_t serial = *(reinterpret_cast<const uint32_t*>(information.serial));

  switch (information.esta_id) {
    case OPEN_LIGHTING_ESTA_ID:
      if (information.device_id == OPEN_LIGHTING_RGB_MIXER_ID ||
          information.device_id == OPEN_LIGHTING_PACKETHEADS_ID) {
        AddDevice(new ArduinoRGBDevice(
            m_plugin_adaptor,
            this,
            device_name,
            widget,
            information.esta_id,
            information.device_id,
            serial));
        return;
      }
      break;
    case JESE_ESTA_ID:
      if (information.device_id == JESE_DMX_TRI_ID) {
        AddDevice(new DmxTriDevice(
            this,
            device_name,
            widget,
            information.esta_id,
            information.device_id,
            serial));
        return;
      }
      break;
    case DMX_KING_ESTA_ID:
      if (information.device_id == DMX_KING_DEVICE_ID) {
        // DMxKing devices are drop in replacements for a Usb Pro
        AddDevice(new UsbProDevice(
            m_plugin_adaptor,
            this,
            device_name,
            widget,
            information.esta_id,
            information.device_id,
            serial,
            GetProFrameLimit()));
        return;
      }
  }
  OLA_WARN << "Defaulting to a Usb Pro device";
  device_name = USBPRO_DEVICE_NAME;
  AddDevice(new UsbProDevice(
        m_plugin_adaptor,
        this,
        device_name,
        widget,
        ENTTEC_ESTA_ID,
        0,  // assume device id is 0
        serial,
        GetProFrameLimit()));
}


/*
 * Add a new device to the list
 * @param device the new UsbDevice
 */
void UsbProPlugin::AddDevice(UsbDevice *device) {
  device->SetOnRemove(NewSingleClosure(this,
                                       &UsbProPlugin::DeviceRemoved,
                                       device));
  m_devices.push_back(device);
  m_plugin_adaptor->RegisterDevice(device);
}


/*
 * Start the plugin
 */
bool UsbProPlugin::StartHook() {
  m_detector.SetListener(this);
  vector<string>::iterator it;
  vector<string> device_paths = FindCandiateDevices();

  for (it = device_paths.begin(); it != device_paths.end(); ++it)
    // NewWidget (above) will be called when discovery completes.
    m_detector.Discover(*it);
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
  if (!device_prefixes.size()) {
    m_preferences->SetMultipleValue(DEVICE_PREFIX_KEY, LINUX_DEVICE_PREFIX);
    m_preferences->SetMultipleValue(DEVICE_PREFIX_KEY, MAC_DEVICE_PREFIX);
    save = true;
  }

  save |= m_preferences->SetDefaultValue(DEVICE_DIR_KEY, StringValidator(),
                                         DEFAULT_DEVICE_DIR);

  save |= m_preferences->SetDefaultValue(USB_PRO_FPS_LIMIT_KEY,
                                         IntValidator(0, MAX_PRO_FPS_LIMIT),
                                         DEFAULT_PRO_FPS_LIMIT);

  if (save)
    m_preferences->Save();

  device_prefixes = m_preferences->GetMultipleValue(DEVICE_PREFIX_KEY);
  if (!device_prefixes.size())
    return false;
  return true;
}


void UsbProPlugin::DeleteDevice(UsbDevice *device) {
  m_plugin_adaptor->UnregisterDevice(device);
  delete device;
}


/*
 * Look for candidate devices in /dev
 * @returns a list of paths that may be usb pro devices
 */
vector<string> UsbProPlugin::FindCandiateDevices() {
  vector<string> device_paths;

  vector<string> device_prefixes =
    m_preferences->GetMultipleValue(DEVICE_PREFIX_KEY);
  string device_dir = m_preferences->GetValue(DEVICE_DIR_KEY);

  if (device_prefixes.size()) {
    DIR *dp;
    struct dirent dir_ent;
    struct dirent *dir_ent_p;
    if ((dp  = opendir(device_dir.data())) == NULL) {
        OLA_WARN << "Could not open " << device_dir << ":" << strerror(errno);
        return device_paths;
    }

    readdir_r(dp, &dir_ent, &dir_ent_p);
    while (dir_ent_p != NULL) {
      vector<string>::const_iterator iter;
      for (iter = device_prefixes.begin(); iter != device_prefixes.end();
           ++iter) {
        if (!strncmp(dir_ent_p->d_name, iter->data(), iter->size())) {
          stringstream str;
          str << device_dir << "/" << dir_ent_p->d_name;
          device_paths.push_back(str.str());
          OLA_INFO << "Found potential USB Pro like device at " << str.str();
        }
      }
      readdir_r(dp, &dir_ent, &dir_ent_p);
    }
    closedir(dp);
  }
  return device_paths;
}


/*
 * Get the Frames per second limit for a pro device
 */
unsigned int UsbProPlugin::GetProFrameLimit() {
  unsigned int fps_limit;
  if (!StringToUInt(m_preferences->GetValue(USB_PRO_FPS_LIMIT_KEY) ,
                    &fps_limit))
    StringToUInt(DEFAULT_PRO_FPS_LIMIT, &fps_limit);
  return fps_limit;
}
}  // usbpro
}  // plugin
}  // ola
