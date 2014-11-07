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
 * UsbDmxPlugin.cpp
 * The UsbDmx plugin for ola
 * Copyright (C) 2006 Simon Newton
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
#include "ola/StringUtils.h"
#include "ola/io/Descriptor.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"

#include "plugins/usbdmx/AnymaDevice.h"
#include "plugins/usbdmx/EuroliteProDevice.h"
#include "plugins/usbdmx/FirmwareLoader.h"
#include "plugins/usbdmx/ScanlimeDevice.h"
#include "plugins/usbdmx/SunliteDevice.h"
#include "plugins/usbdmx/SunliteFirmwareLoader.h"
#include "plugins/usbdmx/UsbDevice.h"
#include "plugins/usbdmx/UsbDmxPlugin.h"
#include "plugins/usbdmx/VellemanDevice.h"


namespace ola {
namespace plugin {
namespace usbdmx {

using ola::io::DeviceDescriptor;
using std::pair;
using std::string;
using std::vector;

const char UsbDmxPlugin::PLUGIN_NAME[] = "USB";
const char UsbDmxPlugin::PLUGIN_PREFIX[] = "usbdmx";
const char UsbDmxPlugin::LIBUSB_DEBUG_LEVEL_KEY[] = "libusb_debug_level";
int UsbDmxPlugin::LIBUSB_DEFAULT_DEBUG_LEVEL = 0;
int UsbDmxPlugin::LIBUSB_MAX_DEBUG_LEVEL = 3;


/*
 * Called by libusb when a new descriptor is created.
 */
void libusb_fd_added(int fd, short events, void *data) {  // NOLINT(runtime/int)
  UsbDmxPlugin *plugin = static_cast<UsbDmxPlugin*>(data);

  OLA_INFO << "USB new FD: " << fd;
  plugin->AddDeviceDescriptor(fd);
  (void) events;
}


/*
 * Called by libusb when a descriptor is no longer needed.
 */
void libusb_fd_removed(int fd, void *data) {
  UsbDmxPlugin *plugin = static_cast<UsbDmxPlugin*>(data);
  OLA_INFO << "USB rm FD: " << fd;
  plugin->RemoveDeviceDescriptor(fd);
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
  if (!StringToInt(m_preferences->GetValue(LIBUSB_DEBUG_LEVEL_KEY) ,
                   &debug_level))
    debug_level = LIBUSB_DEFAULT_DEBUG_LEVEL;

  OLA_DEBUG << "libusb debug level set to " << debug_level;
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
    AddDeviceDescriptor((*pollfds)->fd);
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
      OLA_INFO << "Found a Sunlite device";
      device = new SunliteDevice(this, usb_device);
    } else if (device_descriptor.idVendor == 0x16C0 &&
               device_descriptor.idProduct == 0x05DC) {
      OLA_INFO << "Found an Anyma device";
      device = NewAnymaDevice(usb_device, device_descriptor);
    } else if (device_descriptor.idVendor == 0x04d8 &&
               device_descriptor.idProduct == 0xfa63) {
      OLA_INFO << "Found a EUROLITE device";
       device = new EuroliteProDevice(this, usb_device);
    } else if (device_descriptor.idVendor == 0x1D50 &&
               device_descriptor.idProduct == 0x607A) {
      OLA_INFO << "Found a Scanlime device";
      device = NewScanlimeDevice(usb_device, device_descriptor);
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

  if (!m_descriptors.empty()) {
    OLA_WARN << "libusb is still using descriptor, this is a bug";
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
"Anyma uDMX, Scanlime Fadecandy, Sunlite USBDMX2 & Velleman K8062.\n"
"\n"
"--- Config file : ola-usbdmx.conf ---\n"
"\n"
"libusb_debug_level = {0,1,2,3}\n"
"The debug level for libusb, see http://libusb.sourceforge.net/api-1.0/ .\n"
"0 = No logging, 3 = Verbose.\n"
"\n";
}


/*
 * Default to sensible values
 */
bool UsbDmxPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  bool save = m_preferences->SetDefaultValue(
      LIBUSB_DEBUG_LEVEL_KEY,
      UIntValidator(LIBUSB_DEFAULT_DEBUG_LEVEL, LIBUSB_MAX_DEBUG_LEVEL),
      LIBUSB_DEFAULT_DEBUG_LEVEL);

  if (save)
    m_preferences->Save();

  return true;
}


bool UsbDmxPlugin::AddDeviceDescriptor(int fd) {
  vector<DeviceDescriptor*>::const_iterator iter = m_descriptors.begin();
  for (; iter != m_descriptors.end(); ++iter) {
    if ((*iter)->ReadDescriptor() == fd)
      return true;
  }
  DeviceDescriptor *socket = new DeviceDescriptor(fd);
  socket->SetOnData(NewCallback(this, &UsbDmxPlugin::SocketReady));
  m_plugin_adaptor->AddReadDescriptor(socket);
  m_descriptors.push_back(socket);
  return true;
}


bool UsbDmxPlugin::RemoveDeviceDescriptor(int fd) {
  vector<DeviceDescriptor*>::iterator iter = m_descriptors.begin();
  for (; iter != m_descriptors.end(); ++iter) {
    if ((*iter)->ReadDescriptor() == fd) {
      m_plugin_adaptor->RemoveReadDescriptor(*iter);
      delete *iter;
      m_descriptors.erase(iter);
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


/**
 * Create a new AnymaDevice. Some Anyma devices don't have serial numbers, so
 * we can only support one of those.
 */
UsbDevice* UsbDmxPlugin::NewAnymaDevice(
    libusb_device *usb_device,
    const struct libusb_device_descriptor &device_descriptor) {
  libusb_device_handle *usb_handle;
  if (libusb_open(usb_device, &usb_handle)) {
    OLA_WARN << "Failed to open Anyma usb device";
    return NULL;
  }

  USBDeviceInformation info;
  GetDeviceInfo(usb_handle, device_descriptor, &info);

  if (!MatchManufacturer(AnymaDevice::EXPECTED_MANUFACTURER,
                         info.manufacturer)) {
    libusb_close(usb_handle);
    return NULL;
  }

  if (!MatchProduct(AnymaDevice::EXPECTED_PRODUCT, info.product)) {
    libusb_close(usb_handle);
    return NULL;
  }

  if (info.serial.empty()) {
    if (m_anyma_devices_missing_serial_numbers) {
      OLA_WARN << "Failed to read serial number or serial number empty. "
               << "We can only support one device without a serial number.";
      return NULL;
    } else {
      OLA_WARN << "Failed to read serial number from " << info.manufacturer
               << " : " << info.product
               << " the device probably doesn't have one";
      m_anyma_devices_missing_serial_numbers = true;
    }
  }

  if (libusb_claim_interface(usb_handle, 0)) {
    OLA_WARN << "Failed to claim Anyma usb device";
    libusb_close(usb_handle);
    return NULL;
  }
  return new AnymaDevice(this, usb_device, usb_handle, info.serial);
}


/**
 * Create a new ScanlimeDevice.
 */
UsbDevice* UsbDmxPlugin::NewScanlimeDevice(
    libusb_device *usb_device,
    const struct libusb_device_descriptor &device_descriptor) {
  libusb_device_handle *usb_handle;
  if (libusb_open(usb_device, &usb_handle)) {
    OLA_WARN << "Failed to open Scanlime usb device";
    return NULL;
  }

  USBDeviceInformation info;
  GetDeviceInfo(usb_handle, device_descriptor, &info);

  if (!MatchManufacturer(ScanlimeDevice::EXPECTED_MANUFACTURER,
                         info.manufacturer)) {
    libusb_close(usb_handle);
    return NULL;
  }

  if (!MatchProduct(ScanlimeDevice::EXPECTED_PRODUCT, info.product)) {
    libusb_close(usb_handle);
    return NULL;
  }

  if (info.serial.empty()) {
    OLA_WARN << "Failed to read serial number from " << info.manufacturer
             << " : " << info.product
             << " the device probably doesn't have one";
  }

  if (libusb_claim_interface(usb_handle, 0)) {
    OLA_WARN << "Failed to claim Scanlime usb device";
    libusb_close(usb_handle);
    return NULL;
  }
  return new ScanlimeDevice(this, usb_device, usb_handle, info.serial);
}


/**
 * Get the Manufacturer, Product and Serial number strings for a device.
 */
void UsbDmxPlugin::GetDeviceInfo(
    libusb_device_handle *usb_handle,
    const struct libusb_device_descriptor &device_descriptor,
    USBDeviceInformation *device_info) {
  if (!GetDescriptorString(usb_handle, device_descriptor.iManufacturer,
                           &device_info->manufacturer))
    OLA_INFO << "Failed to get manufacturer name";

  if (!GetDescriptorString(usb_handle, device_descriptor.iProduct,
                           &device_info->product))
    OLA_INFO << "Failed to get product name";

  if (!GetDescriptorString(usb_handle, device_descriptor.iSerialNumber,
                           &device_info->serial))
    OLA_WARN << "Failed to read serial number, the device probably doesn't "
             << "have one";
}


/**
 * Check if the manufacturer string matches the expected value. Log a message
 * if it doesn't.
 */
bool UsbDmxPlugin::MatchManufacturer(const string &expected,
                                     const string &actual) {
  if (expected != actual) {
    OLA_WARN << "Manufacturer mismatch: " << expected << " != " << actual;
    return false;
  }
  return true;
}


/**
 * Check if the manufacturer string matches the expected value. Log a message
 * if it doesn't.
 */
bool UsbDmxPlugin::MatchProduct(const string &expected, const string &actual) {
  if (expected != actual) {
    OLA_WARN << "Product mismatch: " << expected << " != " << actual;
    return false;
  }
  return true;
}

/*
 * Return a string descriptor.
 * @param usb_handle the usb handle to the device
 * @param desc_index the index of the descriptor
 * @param data where to store the output string
 * @returns true if we got the value, false otherwise
 */
bool UsbDmxPlugin::GetDescriptorString(libusb_device_handle *usb_handle,
                                       uint8_t desc_index,
                                       string *data) {
  enum { buffer_size = 32 };  // static arrays FTW!
  unsigned char buffer[buffer_size];
  int r = libusb_get_string_descriptor_ascii(
      usb_handle,
      desc_index,
      buffer,
      buffer_size);

  if (r <= 0) {
    OLA_INFO << "libusb_get_string_descriptor_ascii returned " << r;
    return false;
  }
  data->assign(reinterpret_cast<char*>(buffer));
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
