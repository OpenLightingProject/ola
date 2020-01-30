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

#ifndef LIBS_USB_LIBUSBADAPTOR_H_
#define LIBS_USB_LIBUSBADAPTOR_H_

#include <libusb.h>
#include <string>

#include "ola/base/Macro.h"
#include "libs/usb/Types.h"

namespace ola {
namespace usb {

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

  // Device handling and enumeration

  /**
   * @brief Wraps libusb_ref_device.
   * @param dev the device to reference
   * @returns the same device
   */
  virtual libusb_device* RefDevice(libusb_device *dev) = 0;

  /**
   * @brief Wraps libusb_unref_device.
   * @param dev the device to unreference.
   */
  virtual void UnrefDevice(libusb_device *dev) = 0;

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
  virtual void Close(libusb_device_handle *usb_handle) = 0;

  /**
   * @brief Wraps libusb_set_configuration.
   * @param dev a device handle
   * @param configuration the bConfigurationValue of the configuration you
   *   wish to activate, or -1 if you wish to put the device in an unconfigured
   *   state.
   * @returns 0 on success
   * @returns LIBUSB_ERROR_NOT_FOUND if the requested configuration does not
   *   exist.
   * @returns LIBUSB_ERROR_BUSY if interfaces are currently claimed
   * @returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
   * @returns another LIBUSB_ERROR code on other failure
   */
  virtual int SetConfiguration(libusb_device_handle *dev,
                               int configuration) = 0;

  /**
   * @brief Wraps libusb_claim_interface.
   * @param dev a device handle
   * @param interface_number the <tt>bInterfaceNumber</tt> of the interface you
   * wish to claim
   * @returns 0 on success
   * @returns LIBUSB_ERROR_NOT_FOUND if the requested interface does not exist
   * @returns LIBUSB_ERROR_BUSY if another program or driver has claimed the
   * interface
   * @returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
   * @returns a LIBUSB_ERROR code on other failure
   */
  virtual int ClaimInterface(libusb_device_handle *dev,
                             int interface_number) = 0;

  /**
   * @brief Detach a kernel driver.
   * @param dev a device handle
   * @param interface_number the interface to detach the driver from
   * @returns 0 on success
   * @returns LIBUSB_ERROR_NOT_FOUND if no kernel driver was active
   * @returns LIBUSB_ERROR_INVALID_PARAM if the interface does not exist
   * @returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
   * @returns LIBUSB_ERROR_NOT_SUPPORTED on platforms where the functionality
   * is not available
   * @returns another LIBUSB_ERROR code on other failure
   *
   */
  virtual int DetachKernelDriver(libusb_device_handle *dev,
                                 int interface_number) = 0;

  // USB descriptors
  /**
   * @brief Wraps libusb_get_device_descriptor.
   * @param dev a device
   * @param[out] descriptor The device descriptor.
   * @returns 0 on success
   * @returns another LIBUSB_ERROR code on error
   */
  virtual int GetDeviceDescriptor(
      libusb_device *dev,
      struct libusb_device_descriptor *descriptor) = 0;

  /**
   * @brief Wraps libusb_get_active_config_descriptor.
   * @param dev a device
   * @param[out] config output location for the USB configuration descriptor.
   *   Only valid if 0 was returned. Must be freed with
   *   libusb_free_config_descriptor() after use.
   * @returns 0 on success
   * @returns LIBUSB_ERROR_NOT_FOUND if the device is in unconfigured state
   * @returns another LIBUSB_ERROR code on error
   */
  virtual int GetActiveConfigDescriptor(
      libusb_device *dev,
      struct libusb_config_descriptor **config) = 0;

  /**
   * @brief Wraps libusb_get_config_descriptor.
   * @param dev a device
   * @param config_index the index of the configuration you wish to retrieve
   * @param[out] config output location for the USB configuration descriptor.
   *   Only valid if 0 was returned. Must be freed with
   *   libusb_free_config_descriptor() after use.
   * @returns 0 on success
   * @returns LIBUSB_ERROR_NOT_FOUND if the configuration does not exist
   * @returns another LIBUSB_ERROR code on error
   */
  virtual int GetConfigDescriptor(libusb_device *dev,
                                  uint8_t config_index,
                                  struct libusb_config_descriptor **config) = 0;

  /**
   * @brief Wraps busb_free_config_descriptor.
   * @param config the configuration descriptor to free
   */
  virtual void FreeConfigDescriptor(
      struct libusb_config_descriptor *config) = 0;

  /**
   * @brief Get the value of a string descriptor.
   * @param usb_handle The USB device handle
   * @param descriptor_index The index of the string descriptor to fetch.
   * @param[out] data The value of the string descriptor.
   * @returns true if the string descriptor was retrieved, false otherwise.
   */
  virtual bool GetStringDescriptor(libusb_device_handle *usb_handle,
                                   uint8_t descriptor_index,
                                   std::string *data) = 0;

  // Asynchronous device I/O

  /**
   * @brief Wraps libusb_alloc_transfer
   * @param iso_packets number of isochronous packet descriptors to allocate
   * @returns a newly allocated transfer, or NULL on error
   */
  virtual struct libusb_transfer* AllocTransfer(int iso_packets) = 0;

  /**
   * @brief Wraps libusb_free_transfer.
   * @param transfer the transfer to free
   */
  virtual void FreeTransfer(struct libusb_transfer *transfer) = 0;

  /**
   * @brief Wraps libusb_submit_transfer.
   * @param transfer the transfer to submit
   * @returns 0 on success
   * @returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
   * @returns LIBUSB_ERROR_BUSY if the transfer has already been submitted.
   * @returns LIBUSB_ERROR_NOT_SUPPORTED if the transfer flags are not supported
   *   by the operating system.
   * @returns another LIBUSB_ERROR code on other failure
   */
  virtual int SubmitTransfer(struct libusb_transfer *transfer) = 0;

  /**
   * @brief Wraps libusb_cancel_transfer
   * @param transfer the transfer to cancel
   * @returns 0 on success
   * @returns LIBUSB_ERROR_NOT_FOUND if the transfer is already complete or
   * cancelled.
   * @returns a LIBUSB_ERROR code on failure
   */
  virtual int CancelTransfer(struct libusb_transfer *transfer) = 0;

  /**
   * @brief Wraps libusb_fill_control_setup
   * @param[out] buffer buffer to output the setup packet into
   *  This pointer must be aligned to at least 2 bytes boundary.
   * @param bmRequestType the request type field for the setup packet
   * @param bRequest the request field for the setup packet
   * @param wValue the value field for the setup packet
   * @param wIndex the index field for the setup packet
   * @param wLength the length field for the setup packet. The data buffer
   *   should be at least this size.
   */
  virtual void FillControlSetup(unsigned char *buffer,
                                uint8_t bmRequestType,
                                uint8_t bRequest,
                                uint16_t wValue,
                                uint16_t wIndex,
                                uint16_t wLength) = 0;

  /**
   * @brief Wraps libusb_fill_control_transfer
   * @param[out] transfer the transfer to populate
   * @param dev_handle handle of the device that will handle the transfer
   * @param buffer data buffer. If provided, this function will interpret the
   *   first 8 bytes as a setup packet and infer the transfer length from that.
   *   This pointer must be aligned to at least 2 bytes boundary.
   * @param callback callback function to be invoked on transfer completion
   * @param user_data user data to pass to callback function
   * @param timeout timeout for the transfer in milliseconds
   */
  virtual void FillControlTransfer(struct libusb_transfer *transfer,
                                   libusb_device_handle *dev_handle,
                                   unsigned char *buffer,
                                   libusb_transfer_cb_fn callback,
                                   void *user_data,
                                   unsigned int timeout) = 0;

  /**
   * @brief Wraps libusb_fill_bulk_transfer.
   * @param[out] transfer the transfer to populate
   * @param dev_handle handle of the device that will handle the transfer
   * @param endpoint address of the endpoint where this transfer will be sent
   * @param buffer data buffer. If provided, this function will interpret the
   *   first 8 bytes as a setup packet and infer the transfer length from that.
   *   This pointer must be aligned to at least 2 bytes boundary.
   * @param length length of data buffer
   * @param callback callback function to be invoked on transfer completion
   * @param user_data user data to pass to callback function
   * @param timeout timeout for the transfer in milliseconds
   */
  virtual void FillBulkTransfer(struct libusb_transfer *transfer,
                                libusb_device_handle *dev_handle,
                                unsigned char endpoint,
                                unsigned char *buffer,
                                int length,
                                libusb_transfer_cb_fn callback,
                                void *user_data,
                                unsigned int timeout) = 0;

  /**
   * @brief Wraps libusb_fill_interrupt_transfer.
   * @param[out] transfer the transfer to populate
   * @param dev_handle handle of the device that will handle the transfer
   * @param endpoint address of the endpoint where this transfer will be sent
   * @param buffer data buffer
   * @param length length of data buffer
   * @param callback callback function to be invoked on transfer completion
   * @param user_data user data to pass to callback function
   * @param timeout timeout for the transfer in milliseconds
   */
  virtual void FillInterruptTransfer(struct libusb_transfer *transfer,
                                     libusb_device_handle *dev_handle,
                                     unsigned char endpoint,
                                     unsigned char *buffer,
                                     int length,
                                     libusb_transfer_cb_fn callback,
                                     void *user_data,
                                     unsigned int timeout) = 0;

  // Synchronous device I/O

  /**
   * @brief Wraps libusb_control_transfer().
   * @param dev_handle a handle for the device to communicate with
   * @param bmRequestType the request type field for the setup packet
   * @param bRequest the request field for the setup packet
   * @param wValue the value field for the setup packet
   * @param wIndex the index field for the setup packet
   * @param [in,out] data a suitably-sized data buffer for either input or
   *   output (depending on direction bits within bmRequestType)
   * @param wLength the length field for the setup packet. The data buffer
   *   should be at least this size.
   * @param timeout timeout (in milliseconds) that this function should wait
   *   before giving up due to no response being received. For an unlimited
   *   timeout, use value 0.
   * @returns on success, the number of bytes actually transferred
   *
   */
  virtual int ControlTransfer(libusb_device_handle *dev_handle,
                              uint8_t bmRequestType,
                              uint8_t bRequest,
                              uint16_t wValue,
                              uint16_t wIndex,
                              unsigned char *data,
                              uint16_t wLength,
                              unsigned int timeout) = 0;

  /**
   * @brief Wraps libusb_bulk_transfer.
   * @returns 0 on success and populates <tt>transferred</tt>
   * @returns LIBUSB_ERROR_TIMEOUT if the transfer timed out (and populates
   *   <tt>transferred</tt>)
   * @returns LIBUSB_ERROR_PIPE if the endpoint halted
   * @returns LIBUSB_ERROR_OVERFLOW if the device offered more data, see
   * @returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
   * @returns another LIBUSB_ERROR code on other failures
   */
  virtual int BulkTransfer(struct libusb_device_handle *dev_handle,
                           unsigned char endpoint,
                           unsigned char *data,
                           int length,
                           int *transferred,
                           unsigned int timeout) = 0;

  /**
   * @brief Wraps libusb_interrupt_transfer
   * @returns 0 on success and populates <tt>transferred</tt>
   * @returns LIBUSB_ERROR_TIMEOUT if the transfer timed out
   * @returns LIBUSB_ERROR_PIPE if the endpoint halted
   * @returns LIBUSB_ERROR_OVERFLOW if the device offered more data, see
   * @returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
   * @returns another LIBUSB_ERROR code on other error
   */
  virtual int InterruptTransfer(libusb_device_handle *dev_handle,
                                unsigned char endpoint,
                                unsigned char *data,
                                int length,
                                int *actual_length,
                                unsigned int timeout) = 0;

  /**
   * @brief Get the USBDeviceID for a device.
   * @param device The libusb device to get the ID of.
   * @returns The USBDeviceID for this device.
   */
  virtual USBDeviceID GetDeviceId(libusb_device *device) const = 0;

  // Static helper methods.

  /**
   * @brief Initialize a new libusb context.
   * @param context A pointer to a libusb context.
   * @returns true if the context is initialized, false otherwise.
   */
  static bool Initialize(struct libusb_context **context);

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
   * @param device_info The DeviceInformation struct to check against.
   * @returns true if the strings matched, false otherwise.
   */
  static bool CheckManufacturer(const std::string &expected,
                                const DeviceInformation &device_info);

  /**
   * @brief Check if the product string matches the expected value.
   * @param expected The expected product string.
   * @param device_info The DeviceInformation struct to check against.
   * @returns true if the strings matched, false otherwise.
   */
  static bool CheckProduct(const std::string &expected,
                           const DeviceInformation &device_info);

  /**
   * @brief Check if this platform supports hotplug.
   * @returns true if hotplug is supported and enabled on this platform, false
   *   otherwise.
   */
  static bool HotplugSupported();

  /**
   * @brief Try and convert an error code to a string
   * @param error_code The error code.
   * @returns A string representing the error code.
   */
  static std::string ErrorCodeToString(const int error_code);
};

/**
 * @brief The base LibUsbAdaptor that passes most calls through to libusb.
 */
class BaseLibUsbAdaptor : public LibUsbAdaptor {
 public:
  // Device handling and enumeration
  libusb_device* RefDevice(libusb_device *dev);

  void UnrefDevice(libusb_device *dev);

  int SetConfiguration(libusb_device_handle *dev, int configuration);

  int ClaimInterface(libusb_device_handle *dev, int interface_number);

  int DetachKernelDriver(libusb_device_handle *dev, int interface_number);

  // USB descriptors
  int GetDeviceDescriptor(libusb_device *dev,
                          struct libusb_device_descriptor *descriptor);

  int GetActiveConfigDescriptor(
      libusb_device *dev,
      struct libusb_config_descriptor **config);

  int GetConfigDescriptor(libusb_device *dev,
                          uint8_t config_index,
                          struct libusb_config_descriptor **config);

  void FreeConfigDescriptor(struct libusb_config_descriptor *config);

  bool GetStringDescriptor(libusb_device_handle *usb_handle,
                           uint8_t descriptor_index,
                           std::string *data);

  // Asynchronous device I/O
  struct libusb_transfer* AllocTransfer(int iso_packets);

  void FreeTransfer(struct libusb_transfer *transfer);

  int SubmitTransfer(struct libusb_transfer *transfer);

  int CancelTransfer(struct libusb_transfer *transfer);

  void FillControlSetup(unsigned char *buffer,
                        uint8_t bmRequestType,
                        uint8_t bRequest,
                        uint16_t wValue,
                        uint16_t wIndex,
                        uint16_t wLength);

  void FillControlTransfer(struct libusb_transfer *transfer,
                           libusb_device_handle *dev_handle,
                           unsigned char *buffer,
                           libusb_transfer_cb_fn callback,
                           void *user_data,
                           unsigned int timeout);

  void FillBulkTransfer(struct libusb_transfer *transfer,
                        libusb_device_handle *dev_handle,
                        unsigned char endpoint,
                        unsigned char *buffer,
                        int length,
                        libusb_transfer_cb_fn callback,
                        void *user_data,
                        unsigned int timeout);

  void FillInterruptTransfer(struct libusb_transfer *transfer,
                             libusb_device_handle *dev_handle,
                             unsigned char endpoint,
                             unsigned char *buffer,
                             int length,
                             libusb_transfer_cb_fn callback,
                             void *user_data,
                             unsigned int timeout);

  // Synchronous device I/O
  int ControlTransfer(libusb_device_handle *dev_handle,
                      uint8_t bmRequestType,
                      uint8_t bRequest,
                      uint16_t wValue,
                      uint16_t wIndex,
                      unsigned char *data,
                      uint16_t wLength,
                      unsigned int timeout);

  int BulkTransfer(struct libusb_device_handle *dev_handle,
                   unsigned char endpoint,
                   unsigned char *data,
                   int length,
                   int *transferred,
                   unsigned int timeout);

  int InterruptTransfer(libusb_device_handle *dev_handle,
                        unsigned char endpoint,
                        unsigned char *data,
                        int length,
                        int *actual_length,
                        unsigned int timeout);

  USBDeviceID GetDeviceId(libusb_device *device) const;
};

/**
 * @brief A LibUsbAdaptor for use with Synchronous widgets.
 *
 * When using synchronous mode, we don't have the requirement of interacting
 * with a LibUsbThread.
 */
class SynchronousLibUsbAdaptor : public BaseLibUsbAdaptor {
 public:
  SynchronousLibUsbAdaptor() {}

  bool OpenDevice(libusb_device *usb_device,
                  libusb_device_handle **usb_handle);

  bool OpenDeviceAndClaimInterface(libusb_device *usb_device,
                                   int interface,
                                   libusb_device_handle **usb_handle);

  void Close(libusb_device_handle *usb_handle);

 private:
  DISALLOW_COPY_AND_ASSIGN(SynchronousLibUsbAdaptor);
};

/**
 * @brief A LibUsbAdaptor for use with Asynchronous widgets.
 *
 * Asynchronous mode requires notifying the LibUsbThread when handles are opened
 * and closed.
 */
class AsynchronousLibUsbAdaptor : public BaseLibUsbAdaptor {
 public:
  explicit AsynchronousLibUsbAdaptor(class LibUsbThread *thread)
      : m_thread(thread) {
  }

  bool OpenDevice(libusb_device *usb_device,
                  libusb_device_handle **usb_handle);

  bool OpenDeviceAndClaimInterface(libusb_device *usb_device,
                                   int interface,
                                   libusb_device_handle **usb_handle);

  void Close(libusb_device_handle *usb_handle);

  int ControlTransfer(libusb_device_handle *dev_handle,
                      uint8_t bmRequestType,
                      uint8_t bRequest,
                      uint16_t wValue,
                      uint16_t wIndex,
                      unsigned char *data,
                      uint16_t wLength,
                      unsigned int timeout);

  int BulkTransfer(struct libusb_device_handle *dev_handle,
                   unsigned char endpoint,
                   unsigned char *data,
                   int length,
                   int *transferred,
                   unsigned int timeout);

  int InterruptTransfer(libusb_device_handle *dev_handle,
                        unsigned char endpoint,
                        unsigned char *data,
                        int length,
                        int *actual_length,
                        unsigned int timeout);

 private:
  class LibUsbThread *m_thread;

  DISALLOW_COPY_AND_ASSIGN(AsynchronousLibUsbAdaptor);
};
}  // namespace usb
}  // namespace ola
#endif  // LIBS_USB_LIBUSBADAPTOR_H_
