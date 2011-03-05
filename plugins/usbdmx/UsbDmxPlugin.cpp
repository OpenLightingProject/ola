/*
 *  This dmxgram is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This dmxgram is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this dmxgram; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * UsbDmxPlugin.cpp
 * The UsbDmx plugin for ola
 * Copyright (C) 2006-2007 Simon Newton
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <libusb.h>

#include <string>
#include <utility>
#include <vector>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "ola/network/Socket.h"

#include "plugins/usbdmx/AnymaDevice.h"
#include "plugins/usbdmx/FirmwareLoader.h"
#include "plugins/usbdmx/SunliteDevice.h"
#include "plugins/usbdmx/SunliteFirmwareLoader.h"
#include "plugins/usbdmx/UsbDevice.h"
#include "plugins/usbdmx/UsbDmxPlugin.h"
#include "plugins/usbdmx/VellemanDevice.h"


namespace ola {
namespace plugin {
namespace usbdmx {

using ola::network::DeviceSocket;

const char UsbDmxPlugin::PLUGIN_NAME[] = "USB";
const char UsbDmxPlugin::PLUGIN_PREFIX[] = "usbdmx";
const char UsbDmxPlugin::LIBUSB_DEBUG_LEVEL_KEY[] = "libusb_debug_level";
int UsbDmxPlugin::LIBUSB_DEFAULT_DEBUG_LEVEL = 0;
int UsbDmxPlugin::LIBUSB_MAX_DEBUG_LEVEL = 3;


/*
 * Called by libusb when a new socket is created.
 */
void libusb_fd_added(int fd, short events, void *data) {
  UsbDmxPlugin *plugin = reinterpret_cast<UsbDmxPlugin*>(data);

  OLA_INFO << "USB new FD: " << fd;
  plugin->AddDeviceSocket(fd);
}


/*
 * Called by libusb when a socket is no longer needed.
 */
void libusb_fd_removed(int fd, void *data) {
  UsbDmxPlugin *plugin = reinterpret_cast<UsbDmxPlugin*>(data);
  OLA_INFO << "USB rm FD: " << fd;
  plugin->RemoveDeviceSocket(fd);
}


/*
 * Start the plugin
 */
bool UsbDmxPlugin::StartHook() {
  if (libusb_init(NULL)) {
    OLA_WARN << "Failed to init libusb";
    return false;
  }

  unsigned int debug_level;
  if (!StringToUInt(m_preferences->GetValue(LIBUSB_DEBUG_LEVEL_KEY) ,
                    &debug_level))
    debug_level = LIBUSB_DEFAULT_DEBUG_LEVEL;

  libusb_set_debug(NULL, debug_level);

  if (LoadFirmware()) {
    // we loaded firmware for at least one device, set up a callback to run in
    // a couple of seconds to re-scan for devices
    m_plugin_adaptor->RegisterSingleTimeout(
        3500,
        NewSingleCallback(this, &UsbDmxPlugin::FindDevices));
  }
  FindDevices();

  /*
  if (!libusb_pollfds_handle_timeouts(m_usb_context)) {
    OLA_FATAL << "libusb doesn't have timeout support, BAD";
    return false;
  }

  libusb_set_pollfd_notifiers(m_usb_context,
                              &libusb_fd_added,
                              &libusb_fd_removed,
                              this);

  const libusb_pollfd **pollfds = libusb_get_pollfds(m_usb_context);
  while (*pollfds) {
    OLA_WARN << "poll fd " << (*pollfds)->fd;
    AddDeviceSocket((*pollfds)->fd);
    pollfds++;
  }
  */

  return true;
}


/*
 * Load firmware onto devices if required.
 * @returns true if we loaded firmware for one or more devices
 */
bool UsbDmxPlugin::LoadFirmware() {
  libusb_device **device_list;
  size_t device_count = libusb_get_device_list(NULL, &device_list);
  FirmwareLoader *loader;
  bool loaded = false;

  for (unsigned int i = 0; i < device_count; i++) {
    libusb_device *usb_device = device_list[i];
    loader = NULL;
    struct libusb_device_descriptor device_descriptor;
    libusb_get_device_descriptor(usb_device, &device_descriptor);

    if (device_descriptor.idVendor == 0x0962 &&
        device_descriptor.idProduct == 0x2000) {
      loader = new SunliteFirmwareLoader(usb_device);
    }

    if (loader) {
      loader->LoadFirmware();
      loaded = true;
      delete loader;
    }
  }
  libusb_free_device_list(device_list, 1);  // unref devices
  return loaded;
}


/*
 * Find known devices & register them
 */
void UsbDmxPlugin::FindDevices() {
  libusb_device **device_list;
  libusb_device *found = NULL;
  size_t device_count = libusb_get_device_list(NULL, &device_list);

  for (unsigned int i = 0; i < device_count; i++) {
    libusb_device *usb_device = device_list[i];
    struct libusb_device_descriptor device_descriptor;
    libusb_get_device_descriptor(usb_device, &device_descriptor);
    UsbDevice *device = NULL;

    pair<uint8_t, uint8_t> bus_dev_id(libusb_get_bus_number(usb_device),
                                      libusb_get_device_address(usb_device));

    if ((m_registered_devices.find(bus_dev_id) != m_registered_devices.end()))
      continue;

    if (device_descriptor.idVendor == 0x10cf &&
        device_descriptor.idProduct == 0x8062) {
      OLA_INFO << "Found a Velleman USB device";
      device = new VellemanDevice(this, usb_device);
    } else if (device_descriptor.idVendor == 0x0962 &&
        device_descriptor.idProduct == 0x2001) {
      OLA_INFO << "found a sunlite device";
      device = new SunliteDevice(this, usb_device);
    } else if (device_descriptor.idVendor == 0x16C0 &&
        device_descriptor.idProduct == 0x05DC) {
      OLA_INFO << "found a anyma device";
      device = new AnymaDevice(this, usb_device);
    }

    if (device) {
      if (!device->Start()) {
        delete device;
        continue;
      }
      m_registered_devices.insert(bus_dev_id);
      m_devices.push_back(device);
      m_plugin_adaptor->RegisterDevice(device);
    }
  }
  libusb_free_device_list(device_list, 1);  // unref devices
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool UsbDmxPlugin::StopHook() {
  vector<UsbDevice*>::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->UnregisterDevice(*iter);
    (*iter)->Stop();
    delete *iter;
  }
  m_devices.clear();
  m_registered_devices.clear();

  libusb_exit(NULL);

  if (m_sockets.size()) {
    OLA_WARN << "libusb is still using sockets, this is a bug";
  }
  return true;
}


/*
 * Return the description for this plugin
 */
string UsbDmxPlugin::Description() const {
    return
"USB DMX Plugin\n"
"----------------------------\n"
"\n"
"This plugin supports various USB DMX devices including the \n"
"Anyma uDMX, Sunlite USBDMX2 & Velleman K8062.\n"
"\n"
"--- Config file : ola-usbdmx.conf ---\n"
"\n"
"libusb_debug_level = {0,1,2,3}\n"
"The debug level for libusb, see http://libusb.sourceforge.net/api-1.0/ .\n"
"0 = No logging, 3 = Verbose.\n";
}


/*
 * Default to sensible values
 */
bool UsbDmxPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  bool save = m_preferences->SetDefaultValue(
      LIBUSB_DEBUG_LEVEL_KEY,
      IntValidator(LIBUSB_DEFAULT_DEBUG_LEVEL, LIBUSB_MAX_DEBUG_LEVEL),
      IntToString(LIBUSB_DEFAULT_DEBUG_LEVEL));

  if (save)
    m_preferences->Save();

  return true;
}


bool UsbDmxPlugin::AddDeviceSocket(int fd) {
  vector<DeviceSocket*>::const_iterator iter = m_sockets.begin();
  for (; iter != m_sockets.end(); ++iter) {
    if ((*iter)->ReadDescriptor() == fd)
      return true;
  }
  DeviceSocket *socket = new DeviceSocket(fd);
  socket->SetOnData(NewCallback(this, &UsbDmxPlugin::SocketReady));
  m_plugin_adaptor->AddSocket(socket);
  m_sockets.push_back(socket);
}


bool UsbDmxPlugin::RemoveDeviceSocket(int fd) {
  vector<DeviceSocket*>::iterator iter = m_sockets.begin();
  for (; iter != m_sockets.end(); ++iter) {
    if ((*iter)->ReadDescriptor() == fd) {
      m_plugin_adaptor->RemoveSocket(*iter);
      delete *iter;
      m_sockets.erase(iter);
      return true;
    }
  }
  return false;
}


/*
 * Called when there is activity on one of our sockets.
 */
void UsbDmxPlugin::SocketReady() {
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  libusb_handle_events_timeout(NULL, &tv);
}
}  // usbdmx
}  // plugin
}  // ola
