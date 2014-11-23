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
 * LibUsbAdaptor.h
 * The wrapper around libusb calls.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_LIBUSBADAPTOR_H_
#define PLUGINS_USBDMX_LIBUSBADAPTOR_H_

#include <libusb.h>
#include <string>

#include "ola/base/Macro.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Wraps calls to libusb so we can test the code.
 */
class LibUsbAdaptor {
 public:
  struct DeviceInformation {
    std::string manufacturer;
    std::string product;
    std::string serial;
  };

  virtual ~LibUsbAdaptor() {}

  /**
   * @brief Open a libusb device.
   * @param usb_device The usb device to open.
   * @param[out] usb_handle the new device handle.
   * @returns true if the device was opened, false otherwise.
   */
  virtual bool OpenDevice(libusb_device *usb_device,
                          libusb_device_handle **usb_handle) = 0;

  /**
   * @brief Open a libusb device and claim an interface.
   * @param usb_device The usb device to open.
   * @param interface the interface index to claim.
   * @param[out] usb_handle the new device handle.
   * @returns true if the device was opened and the interface claimed,
   *   false otherwise.
   */
  virtual bool OpenDeviceAndClaimInterface(
      libusb_device *usb_device,
      int interface,
      libusb_device_handle **usb_handle) = 0;

  /**
   * @brief Close a libusb handle.
   * @param usb_handle the handle to close.
   */
  virtual void CloseHandle(libusb_device_handle *usb_handle) = 0;

  /**
   * @brief Fetch the manufacturer, product and serial strings from a device.
   * @param usb_device The USB device to get information for.
   * @param device_descriptor The descriptor to use
   * @param[out] device_info The DeviceInformation struct to populate.
   * @returns true if we fetched the information, false otherwise.
   */
  static bool GetDeviceInfo(
      struct libusb_device *usb_device,
      const struct libusb_device_descriptor &device_descriptor,
      DeviceInformation *device_info);

  /**
   * @brief Check if the manufacturer string matches the expected value.
   * @param expected The expected manufacturer string.
   * @param actual The actual manufacturer string.
   * @returns true if the strings matched, false otherwise.
   */
  static bool CheckManufacturer(const std::string &expected,
                                const std::string &actual);

  /**
   * @brief Check if the product string matches the expected value.
   * @param expected The expected product string.
   * @param actual The actual product string.
   * @returns true if the strings matched, false otherwise.
   */
  static bool CheckProduct(const std::string &expected,
                           const std::string &actual);
};


/**
 * @brief A LibUsbAdaptor for use with Syncronous widgets.
 *
 * When using syncronous mode, we don't have the requirement of interacting
 * with a LibUsbThread.
 */
class SyncronousLibUsbAdaptor : public LibUsbAdaptor {
 public:
  SyncronousLibUsbAdaptor() {}

  bool OpenDevice(libusb_device *usb_device,
                  libusb_device_handle **usb_handle);

  bool OpenDeviceAndClaimInterface(libusb_device *usb_device,
                                   int interface,
                                   libusb_device_handle **usb_handle);

  void CloseHandle(libusb_device_handle *usb_handle);

 private:
  DISALLOW_COPY_AND_ASSIGN(SyncronousLibUsbAdaptor);
};

/**
 * @brief A LibUsbAdaptor for use with Asyncronous widgets.
 *
 * Asyncronous mode requires notifying the LibUsbThread when handles are opened
 * and closed.
 */
class AsyncronousLibUsbAdaptor : public LibUsbAdaptor {
 public:
  explicit AsyncronousLibUsbAdaptor(class LibUsbThread *thread);

  bool OpenDevice(libusb_device *usb_device,
                  libusb_device_handle **usb_handle);

  bool OpenDeviceAndClaimInterface(libusb_device *usb_device,
                                   int interface,
                                   libusb_device_handle **usb_handle);

  void CloseHandle(libusb_device_handle *usb_handle);

 private:
  class LibUsbThread *m_thread;

  DISALLOW_COPY_AND_ASSIGN(AsyncronousLibUsbAdaptor);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_LIBUSBADAPTOR_H_
