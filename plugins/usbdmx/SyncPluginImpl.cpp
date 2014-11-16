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
#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"
#include "olad/PluginAdaptor.h"

#include "plugins/usbdmx/AnymaWidget.h"
#include "plugins/usbdmx/EuroliteProWidget.h"
#include "plugins/usbdmx/FirmwareLoader.h"
#include "plugins/usbdmx/GenericDevice.h"
#include "plugins/usbdmx/SunliteFirmwareLoader.h"
#include "plugins/usbdmx/SunliteWidget.h"
#include "plugins/usbdmx/VellemanWidget.h"


namespace ola {
namespace plugin {
namespace usbdmx {

using ola::io::DeviceDescriptor;
using std::pair;
using std::string;
using std::vector;

SyncPluginImpl::SyncPluginImpl(PluginAdaptor *plugin_adaptor,
                               Plugin *plugin,
                               unsigned int debug_level)
    : m_plugin_adaptor(plugin_adaptor),
      m_plugin(plugin),
      m_debug_level(debug_level),
      m_context(NULL),
      m_anyma_devices_missing_serial_numbers(false) {
}

bool SyncPluginImpl::Start() {
  if (libusb_init(&m_context)) {
    OLA_WARN << "Failed to init libusb";
    return false;
  }

  OLA_DEBUG << "libusb debug level set to " << m_debug_level;
  libusb_set_debug(m_context, m_debug_level);

  if (LoadFirmware()) {
    // we loaded firmware for at least one device, set up a callback to run in
    // a couple of seconds to re-scan for devices
    m_plugin_adaptor->RegisterSingleTimeout(
        3500,
        NewSingleCallback(this, &SyncPluginImpl::FindDevices));
  }
  FindDevices();
  return true;
}

bool SyncPluginImpl::Stop() {
  vector<Device*>::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->UnregisterDevice(*iter);
    (*iter)->Stop();
    delete *iter;
  }
  m_devices.clear();
  m_registered_devices.clear();

  libusb_exit(m_context);

  return true;
}



/*
 * Load firmware onto devices if required.
 * @returns true if we loaded firmware for one or more devices
 */
bool SyncPluginImpl::LoadFirmware() {
  libusb_device **device_list;
  size_t device_count = libusb_get_device_list(m_context, &device_list);
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
void SyncPluginImpl::FindDevices() {
  libusb_device **device_list;
  size_t device_count = libusb_get_device_list(m_context, &device_list);

  for (unsigned int i = 0; i < device_count; i++) {
    CheckDevice(device_list[i]);
  }
  libusb_free_device_list(device_list, 1);  // unref devices
}

void SyncPluginImpl::CheckDevice(libusb_device *usb_device) {
  struct libusb_device_descriptor device_descriptor;
  libusb_get_device_descriptor(usb_device, &device_descriptor);
  Device *device = NULL;

  pair<uint8_t, uint8_t> bus_dev_id(libusb_get_bus_number(usb_device),
                                    libusb_get_device_address(usb_device));

  if (STLContains(m_registered_devices, bus_dev_id)) {
    return;
  }

  if (device_descriptor.idVendor == 0x10cf &&
      device_descriptor.idProduct == 0x8062) {
    OLA_INFO << "Found a Velleman USB device";
    SynchronousVellemanWidget *widget = new SynchronousVellemanWidget(
        &m_usb_adaptor, usb_device);
    if (!widget->Init()) {
      delete widget;
      return;
    }
    device = new GenericDevice(
        m_plugin, widget, "Velleman USB Device", "velleman");

  } else if (device_descriptor.idVendor == 0x0962 &&
             device_descriptor.idProduct == 0x2001) {
    OLA_INFO << "Found a Sunlite device";
    SynchronousSunliteWidget *widget = new SynchronousSunliteWidget(
        &m_usb_adaptor, usb_device);
    if (!widget->Init()) {
      delete widget;
      return;
    }
    device = new GenericDevice(
        m_plugin, widget, "Sunlite USBDMX2 Device", "usbdmx2");

  } else if (device_descriptor.idVendor == 0x16C0 &&
             device_descriptor.idProduct == 0x05DC) {
    OLA_INFO << "Found an Anyma device";
    device = NewAnymaDevice(usb_device, device_descriptor);
  } else if (device_descriptor.idVendor == 0x04d8 &&
             device_descriptor.idProduct == 0xfa63) {
    OLA_INFO << "Found a EUROLITE device";
    device = NewEuroliteProDevice(usb_device);
  }

  if (device) {
    if (!device->Start()) {
      delete device;
      return;
    }
    m_registered_devices.insert(bus_dev_id);
    m_devices.push_back(device);
    m_plugin_adaptor->RegisterDevice(device);
  }
}

/**
 * Create a new AnymaDevice. Some Anyma devices don't have serial numbers, so
 * we can only support one of those.
 */
Device* SyncPluginImpl::NewAnymaDevice(
    libusb_device *usb_device,
    const struct libusb_device_descriptor &device_descriptor) {
  libusb_device_handle *usb_handle;
  if (libusb_open(usb_device, &usb_handle)) {
    OLA_WARN << "Failed to open Anyma usb device";
    return NULL;
  }

  USBDeviceInformation info;
  GetDeviceInfo(usb_handle, device_descriptor, &info);

  if (!MatchManufacturer(AnymaWidget::EXPECTED_MANUFACTURER,
                        info.manufacturer)) {
    libusb_close(usb_handle);
    return NULL;
  }

  if (!MatchProduct(AnymaWidget::EXPECTED_PRODUCT, info.product)) {
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

  SynchronousAnymaWidget *widget = new SynchronousAnymaWidget(
      &m_usb_adaptor, usb_device, info.serial);
  if (!widget->Init()) {
    delete widget;
    return NULL;
  }
  return new GenericDevice(
      m_plugin, widget, "Anyma USB Device", "anyma-" + widget->SerialNumber());
}

Device* SyncPluginImpl::NewEuroliteProDevice(
    struct libusb_device *usb_device) {

  // There is no Serialnumber--> work around: bus+device number
  int bus_number = libusb_get_bus_number(usb_device);
  int device_address = libusb_get_device_address(usb_device);

  OLA_INFO << "Bus_number: " <<  bus_number << ", Device_address: " <<
    device_address;

  std::ostringstream serial_str;
  serial_str << bus_number << "-" << device_address;

  SynchronousEuroliteProWidget *widget = new SynchronousEuroliteProWidget(
      &m_usb_adaptor, usb_device, serial_str.str());
  if (!widget->Init()) {
    delete widget;
    return NULL;
  }

  return new GenericDevice(
      m_plugin, widget, "EurolitePro USB Device",
      "eurolite-" + widget->SerialNumber());
}

/**
 * Get the Manufacturer, Product and Serial number strings for a device.
 */
void SyncPluginImpl::GetDeviceInfo(
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
bool SyncPluginImpl::MatchManufacturer(const string &expected,
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
bool SyncPluginImpl::MatchProduct(const string &expected,
                                  const string &actual) {
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
bool SyncPluginImpl::GetDescriptorString(libusb_device_handle *usb_handle,
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
