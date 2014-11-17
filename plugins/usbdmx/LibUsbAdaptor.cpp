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
 * LibUsbAdaptor.cpp
 * The wrapper around libusb calls.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/LibUsbAdaptor.h"

#include <libusb.h>
#include <ola/Logging.h>
#include <string>

#include "plugins/usbdmx/LibUsbThread.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;

namespace {

/**
 * @brief A wrapper around libusb_get_string_descriptor_ascii.
 */
bool GetStringDescriptorAscii(libusb_device_handle *usb_handle,
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
    OLA_INFO << "libusb_get_string_descriptor_ascii failed: "
             << libusb_error_name(r);
    return false;
  }
  data->assign(reinterpret_cast<char*>(buffer));
  return true;
}

/**
 * @brief A wrapper around libusb_open.
 */
bool Open(libusb_device *usb_device,
          libusb_device_handle **usb_handle) {
  int r = libusb_open(usb_device, usb_handle);
  if (r) {
    OLA_WARN << "Failed to open libusb device: " << usb_device << ": "
             << libusb_error_name(r);;
    return false;
  }
  return true;
}

/**
 * @brief A wrapper around libusb_close.
 */
void Close(libusb_device_handle *usb_handle) {
  libusb_close(usb_handle);
}

bool OpenHandleAndClaimInterface(
    libusb_device *usb_device,
    int interface,
    libusb_device_handle **usb_handle) {
  if (!Open(usb_device, usb_handle)) {
    return false;
  }

  int r = libusb_claim_interface(*usb_handle, 0);
  if (r) {
    OLA_WARN << "Failed to claim interface " << interface
             << " on device: " << usb_device << ": "
             << libusb_error_name(r);
    Close(*usb_handle);
    return false;
  }
  return true;
}
}  // namespace

// LibUsbAdaptor
// ----------------------------------------------------------------------------

bool LibUsbAdaptor::GetDeviceInfo(
    struct libusb_device *usb_device,
    const struct libusb_device_descriptor &device_descriptor,
    DeviceInformation *device_info) {
  // Since the calls on the handle are syncronous, we don't bother adding the
  // handle to the thread.
  libusb_device_handle *usb_handle;
  if (!Open(usb_device, &usb_handle)) {
    return false;
  }

  if (!GetStringDescriptorAscii(usb_handle, device_descriptor.iManufacturer,
                                &device_info->manufacturer)) {
    OLA_INFO << "Failed to get manufacturer name";
  }

  if (!GetStringDescriptorAscii(usb_handle, device_descriptor.iProduct,
                                &device_info->product)) {
    OLA_INFO << "Failed to get product name";
  }

  if (!GetStringDescriptorAscii(usb_handle, device_descriptor.iSerialNumber,
                                &device_info->serial)) {
    OLA_WARN << "Failed to read serial number, the device probably doesn't "
             << "have one";
  }

  Close(usb_handle);
  return true;
}

bool LibUsbAdaptor::CheckManufacturer(const string &expected,
                                      const string &actual) {
  if (expected != actual) {
    OLA_WARN << "Manufacturer mismatch: " << expected << " != " << actual;
    return false;
  }
  return true;
}

bool LibUsbAdaptor::CheckProduct(const string &expected, const string &actual) {
  if (expected != actual) {
    OLA_WARN << "Product mismatch: " << expected << " != " << actual;
    return false;
  }
  return true;
}

// SyncronousLibUsbAdaptor
// -----------------------------------------------------------------------------

bool SyncronousLibUsbAdaptor::OpenDevice(
    libusb_device *usb_device,
    libusb_device_handle **usb_handle) {
  return Open(usb_device, usb_handle);
}

bool SyncronousLibUsbAdaptor::OpenDeviceAndClaimInterface(
    libusb_device *usb_device,
    int interface,
    libusb_device_handle **usb_handle) {
  return OpenHandleAndClaimInterface(usb_device, interface, usb_handle);
}

void SyncronousLibUsbAdaptor::CloseHandle(libusb_device_handle *usb_handle) {
  Close(usb_handle);
}

// AsyncronousLibUsbAdaptor
// -----------------------------------------------------------------------------

AsyncronousLibUsbAdaptor::AsyncronousLibUsbAdaptor(LibUsbThread *thread)
    : m_thread(thread) {
}

bool AsyncronousLibUsbAdaptor::OpenDevice(libusb_device *usb_device,
                                          libusb_device_handle **usb_handle) {
  bool ok = Open(usb_device, usb_handle);
  if (ok) {
    m_thread->OpenHandle();
  }
  return ok;
}

bool AsyncronousLibUsbAdaptor::OpenDeviceAndClaimInterface(
    libusb_device *usb_device,
    int interface,
    libusb_device_handle **usb_handle) {
  bool ok = OpenHandleAndClaimInterface(usb_device, interface, usb_handle);
  if (ok) {
    m_thread->OpenHandle();
  }
  return ok;
}

void AsyncronousLibUsbAdaptor::CloseHandle(libusb_device_handle *handle) {
  m_thread->CloseHandle(handle);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
