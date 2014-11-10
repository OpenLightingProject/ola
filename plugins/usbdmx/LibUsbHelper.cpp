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
 * LibUsbHelper.cpp
 * Helper methods for libusb device enumeration.
 * Copyright (C) 2014 Simon Newton
 */
#include "plugins/usbdmx/LibUsbHelper.h"

#include <libusb.h>
#include <string>
#include "ola/Logging.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;

namespace {
bool GetDescriptorString(libusb_device_handle *usb_handle,
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
}  // namespace

bool LibUsbHelper::GetDeviceInfo(
    struct libusb_device *usb_device,
    const struct libusb_device_descriptor &device_descriptor,
    DeviceInformation *device_info) {
  libusb_device_handle *usb_handle;
  if (!OpenDevice(usb_device, &usb_handle)) {
    return false;
  }

  if (!GetDescriptorString(usb_handle, device_descriptor.iManufacturer,
                           &device_info->manufacturer)) {
    OLA_INFO << "Failed to get manufacturer name";
  }

  if (!GetDescriptorString(usb_handle, device_descriptor.iProduct,
                           &device_info->product)) {
    OLA_INFO << "Failed to get product name";
  }

  if (!GetDescriptorString(usb_handle, device_descriptor.iSerialNumber,
                           &device_info->serial)) {
    OLA_WARN << "Failed to read serial number, the device probably doesn't "
             << "have one";
  }

  libusb_close(usb_handle);
  return true;
}

bool LibUsbHelper::CheckManufacturer(const string &expected,
                                     const string &actual) {
  if (expected != actual) {
    OLA_WARN << "Manufacturer mismatch: " << expected << " != " << actual;
    return false;
  }
  return true;
}

bool LibUsbHelper::CheckProduct(const string &expected, const string &actual) {
  if (expected != actual) {
    OLA_WARN << "Product mismatch: " << expected << " != " << actual;
    return false;
  }
  return true;
}

bool LibUsbHelper::OpenDevice(libusb_device *usb_device,
                              libusb_device_handle **usb_handle) {
  if (libusb_open(usb_device, usb_handle)) {
    OLA_WARN << "Failed to open libusb device: " << usb_device;
    return false;
  }
  return true;
}

bool LibUsbHelper::OpenDeviceAndClaimInterface(
    libusb_device *usb_device,
    int interface,
    libusb_device_handle **usb_handle) {
  if (!OpenDevice(usb_device, usb_handle)) {
    return false;
  }

  if (libusb_claim_interface(*usb_handle, 0)) {
    OLA_WARN << "Failed to claim interface " << interface
             << " for libusb device: " << usb_device;
    libusb_close(*usb_handle);
    return false;
  }
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
