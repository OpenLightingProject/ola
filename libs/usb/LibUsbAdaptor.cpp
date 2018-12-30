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

#include "libs/usb/LibUsbAdaptor.h"

#include <libusb.h>
#include <ola/Logging.h>
#include <sstream>
#include <string>

#include "libs/usb/LibUsbThread.h"

namespace ola {
namespace usb {

using std::ostringstream;
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
             << LibUsbAdaptor::ErrorCodeToString(r);
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
             << LibUsbAdaptor::ErrorCodeToString(r);
    return false;
  }
  return true;
}

bool OpenHandleAndClaimInterface(libusb_device *usb_device,
                                 int interface,
                                 libusb_device_handle **usb_handle) {
  if (!Open(usb_device, usb_handle)) {
    return false;
  }

  int r = libusb_claim_interface(*usb_handle, interface);
  if (r == LIBUSB_ERROR_BUSY) {
    int error = libusb_detach_kernel_driver(*usb_handle, interface);
    if (error) {
      OLA_WARN << "Failed to detach kernel driver for interface " << interface
               << " on device: " << usb_device;
    } else {
      r = libusb_claim_interface(*usb_handle, interface);
    }
  }
  if (r) {
    OLA_WARN << "Failed to claim interface " << interface
             << " on device: " << usb_device << ": "
             << LibUsbAdaptor::ErrorCodeToString(r);
    libusb_close(*usb_handle);
    *usb_handle = NULL;
    return false;
  }
  return true;
}
}  // namespace

// LibUsbAdaptor
// ----------------------------------------------------------------------------

bool LibUsbAdaptor::Initialize(struct libusb_context **context) {
  int r = libusb_init(context);
  if (r) {
    OLA_WARN << "libusb_init() failed: " << ErrorCodeToString(r);
    return false;
  }
  return true;
}

bool LibUsbAdaptor::GetDeviceInfo(
    struct libusb_device *usb_device,
    const struct libusb_device_descriptor &device_descriptor,
    DeviceInformation *device_info) {
  // Since the calls on the handle are synchronous, we don't bother adding the
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

  libusb_close(usb_handle);
  return true;
}

bool LibUsbAdaptor::CheckManufacturer(const string &expected,
                                      const DeviceInformation &device_info) {
  if (expected != device_info.manufacturer) {
    OLA_WARN << "Manufacturer mismatch: " << expected << " != "
             << device_info.manufacturer;
    return false;
  }
  return true;
}

bool LibUsbAdaptor::CheckProduct(const string &expected,
                                 const DeviceInformation &device_info) {
  if (expected != device_info.product) {
    OLA_WARN << "Product mismatch: " << expected << " != "
             << device_info.product;
    return false;
  }
  return true;
}

bool LibUsbAdaptor::HotplugSupported() {
#ifdef HAVE_LIBUSB_HOTPLUG_API
  return libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) != 0;
#else
  return false;
#endif  // HAVE_LIBUSB_HOTPLUG_API
}

string LibUsbAdaptor::ErrorCodeToString(const int error_code) {
#ifdef HAVE_LIBUSB_ERROR_NAME
  return libusb_error_name(error_code);
#else
  ostringstream str;
  str << "Error code " << error_code;
  return str.str();
#endif  // HAVE_LIBUSB_ERROR_NAME
}

// BaseLibUsbAdaptor
// ----------------------------------------------------------------------------
libusb_device* BaseLibUsbAdaptor::RefDevice(libusb_device *dev) {
  return libusb_ref_device(dev);
}

void BaseLibUsbAdaptor::UnrefDevice(libusb_device *dev) {
  libusb_unref_device(dev);
}

int BaseLibUsbAdaptor::SetConfiguration(libusb_device_handle *dev,
                                        int configuration) {
  return libusb_set_configuration(dev, configuration);
}

int BaseLibUsbAdaptor::ClaimInterface(libusb_device_handle *dev,
                                      int interface_number) {
  return libusb_claim_interface(dev, interface_number);
}

int BaseLibUsbAdaptor::DetachKernelDriver(libusb_device_handle *dev,
                                          int interface_number) {
  if (libusb_kernel_driver_active(dev, interface_number)) {
    int r = libusb_detach_kernel_driver(dev, interface_number);
    if (r) {
      OLA_WARN << "libusb_detach_kernel_driver failed for: " << dev
               << ": " << LibUsbAdaptor::ErrorCodeToString(r);
    }
    return r;
  } else {
    return 0;
  }
}

int BaseLibUsbAdaptor::GetActiveConfigDescriptor(
      libusb_device *dev,
      struct libusb_config_descriptor **config) {
  int r = libusb_get_active_config_descriptor(dev, config);
  if (r) {
    OLA_WARN << "libusb_get_active_config_descriptor failed for: " << dev
             << ": " << LibUsbAdaptor::ErrorCodeToString(r);
  }
  return r;
}

int BaseLibUsbAdaptor::GetDeviceDescriptor(
    libusb_device *dev,
    struct libusb_device_descriptor *desc) {
  int r = libusb_get_device_descriptor(dev, desc);
  if (r) {
    OLA_WARN << "libusb_get_device_descriptor failed for: " << dev
             << ": " << LibUsbAdaptor::ErrorCodeToString(r);
  }
  return r;
}

int BaseLibUsbAdaptor::GetConfigDescriptor(
    libusb_device *dev,
    uint8_t config_index,
    struct libusb_config_descriptor **config) {
  int r = libusb_get_config_descriptor(dev, config_index, config);
  if (r) {
    OLA_WARN << "libusb_get_config_descriptor failed for: " << dev
             << ": " << LibUsbAdaptor::ErrorCodeToString(r);
  }
  return r;
}

void BaseLibUsbAdaptor::FreeConfigDescriptor(
    struct libusb_config_descriptor *config) {
  libusb_free_config_descriptor(config);
}

bool BaseLibUsbAdaptor::GetStringDescriptor(
    libusb_device_handle *usb_handle,
    uint8_t descriptor_index,
    string *data) {
  return GetStringDescriptorAscii(usb_handle, descriptor_index, data);
}

struct libusb_transfer* BaseLibUsbAdaptor::AllocTransfer(int iso_packets) {
  return libusb_alloc_transfer(iso_packets);
}

void BaseLibUsbAdaptor::FreeTransfer(struct libusb_transfer *transfer) {
  return libusb_free_transfer(transfer);
}

int BaseLibUsbAdaptor::SubmitTransfer(struct libusb_transfer *transfer) {
  return libusb_submit_transfer(transfer);
}

int BaseLibUsbAdaptor::CancelTransfer(struct libusb_transfer *transfer) {
  return libusb_cancel_transfer(transfer);
}

void BaseLibUsbAdaptor::FillControlSetup(unsigned char *buffer,
                                         uint8_t bmRequestType,
                                         uint8_t bRequest,
                                         uint16_t wValue,
                                         uint16_t wIndex,
                                         uint16_t wLength) {
  return libusb_fill_control_setup(buffer, bmRequestType, bRequest, wValue,
                                   wIndex, wLength);
}

void BaseLibUsbAdaptor::FillControlTransfer(
    struct libusb_transfer *transfer,
    libusb_device_handle *dev_handle,
    unsigned char *buffer,
    libusb_transfer_cb_fn callback,
    void *user_data,
    unsigned int timeout) {
  return libusb_fill_control_transfer(transfer, dev_handle, buffer, callback,
                                      user_data, timeout);
}

void BaseLibUsbAdaptor::FillBulkTransfer(struct libusb_transfer *transfer,
                                         libusb_device_handle *dev_handle,
                                         unsigned char endpoint,
                                         unsigned char *buffer,
                                         int length,
                                         libusb_transfer_cb_fn callback,
                                         void *user_data,
                                         unsigned int timeout) {
  libusb_fill_bulk_transfer(transfer, dev_handle, endpoint, buffer,
                            length, callback, user_data, timeout);
}

void BaseLibUsbAdaptor::FillInterruptTransfer(struct libusb_transfer *transfer,
                                              libusb_device_handle *dev_handle,
                                              unsigned char endpoint,
                                              unsigned char *buffer,
                                              int length,
                                              libusb_transfer_cb_fn callback,
                                              void *user_data,
                                              unsigned int timeout) {
  libusb_fill_interrupt_transfer(transfer, dev_handle, endpoint, buffer,
                                 length, callback, user_data, timeout);
}

int BaseLibUsbAdaptor::ControlTransfer(
    libusb_device_handle *dev_handle,
    uint8_t bmRequestType,
    uint8_t bRequest,
    uint16_t wValue,
    uint16_t wIndex,
    unsigned char *data,
    uint16_t wLength,
    unsigned int timeout) {
  return libusb_control_transfer(dev_handle, bmRequestType, bRequest, wValue,
                                 wIndex, data, wLength, timeout);
}

int BaseLibUsbAdaptor::BulkTransfer(struct libusb_device_handle *dev_handle,
                                    unsigned char endpoint,
                                    unsigned char *data,
                                    int length,
                                    int *transferred,
                                    unsigned int timeout) {
  return libusb_bulk_transfer(dev_handle, endpoint, data, length, transferred,
                              timeout);
}

int BaseLibUsbAdaptor::InterruptTransfer(libusb_device_handle *dev_handle,
                                         unsigned char endpoint,
                                         unsigned char *data,
                                         int length,
                                         int *actual_length,
                                         unsigned int timeout) {
  return libusb_interrupt_transfer(dev_handle, endpoint, data, length,
                                   actual_length, timeout);
}

USBDeviceID BaseLibUsbAdaptor::GetDeviceId(libusb_device *device) const {
  return USBDeviceID(libusb_get_bus_number(device),
                     libusb_get_device_address(device));
}

// SynchronousLibUsbAdaptor
// -----------------------------------------------------------------------------
bool SynchronousLibUsbAdaptor::OpenDevice(libusb_device *usb_device,
                                          libusb_device_handle **usb_handle) {
  return Open(usb_device, usb_handle);
}

bool SynchronousLibUsbAdaptor::OpenDeviceAndClaimInterface(
    libusb_device *usb_device,
    int interface,
    libusb_device_handle **usb_handle) {
  return OpenHandleAndClaimInterface(usb_device, interface, usb_handle);
}

void SynchronousLibUsbAdaptor::Close(libusb_device_handle *usb_handle) {
  libusb_close(usb_handle);
}

// AsynchronousLibUsbAdaptor
// -----------------------------------------------------------------------------
bool AsynchronousLibUsbAdaptor::OpenDevice(libusb_device *usb_device,
                                           libusb_device_handle **usb_handle) {
  bool ok = Open(usb_device, usb_handle);
  if (ok) {
    m_thread->OpenHandle();
  }
  return ok;
}

bool AsynchronousLibUsbAdaptor::OpenDeviceAndClaimInterface(
    libusb_device *usb_device,
    int interface,
    libusb_device_handle **usb_handle) {
  bool ok = OpenHandleAndClaimInterface(usb_device, interface, usb_handle);
  if (ok) {
    m_thread->OpenHandle();
  }
  return ok;
}

void AsynchronousLibUsbAdaptor::Close(libusb_device_handle *handle) {
  m_thread->CloseHandle(handle);
}

int AsynchronousLibUsbAdaptor::ControlTransfer(
    OLA_UNUSED libusb_device_handle *dev_handle,
    OLA_UNUSED uint8_t bmRequestType,
    OLA_UNUSED uint8_t bRequest,
    OLA_UNUSED uint16_t wValue,
    OLA_UNUSED uint16_t wIndex,
    OLA_UNUSED unsigned char *data,
    OLA_UNUSED uint16_t wLength,
    OLA_UNUSED unsigned int timeout) {
  OLA_DEBUG << "libusb_control_transfer in an AsynchronousLibUsbAdaptor";
  return BaseLibUsbAdaptor::ControlTransfer(dev_handle, bmRequestType,
                                            bRequest, wValue, wIndex, data,
                                            wLength, timeout);
}

int AsynchronousLibUsbAdaptor::BulkTransfer(
    struct libusb_device_handle *dev_handle,
    unsigned char endpoint,
    unsigned char *data,
    int length,
    int *transferred,
    unsigned int timeout) {
  OLA_DEBUG << "libusb_bulk_transfer in an AsynchronousLibUsbAdaptor";
  return BaseLibUsbAdaptor::BulkTransfer(dev_handle, endpoint, data, length,
                                         transferred, timeout);
}

int AsynchronousLibUsbAdaptor::InterruptTransfer(
    OLA_UNUSED libusb_device_handle *dev_handle,
    OLA_UNUSED unsigned char endpoint,
    OLA_UNUSED unsigned char *data,
    OLA_UNUSED int length,
    OLA_UNUSED int *actual_length,
    OLA_UNUSED unsigned int timeout) {
  OLA_DEBUG << "libusb_interrupt_transfer in an AsynchronousLibUsbAdaptor";
  return BaseLibUsbAdaptor::InterruptTransfer(dev_handle, endpoint, data,
                                              length, actual_length, timeout);
}
}  // namespace usb
}  // namespace ola
