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
 * AsyncUsbTransceiverBase.h
 * An Asynchronous DMX USB transceiver.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_ASYNCUSBTRANSCEIVERBASE_H_
#define PLUGINS_USBDMX_ASYNCUSBTRANSCEIVERBASE_H_

#include <libusb.h>

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/DmxBuffer.h"
#include "ola/base/Macro.h"
#include "ola/thread/Mutex.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief A base class that implements common functionality to send or receive
 * DMX asynchronously to a libusb_device.
 */
class AsyncUsbTransceiverBase {
 public:
  /**
   * @brief Create a new AsyncUsbTransceiverBase.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   */
  AsyncUsbTransceiverBase(ola::usb::LibUsbAdaptor* const adaptor,
                          libusb_device *usb_device);

  /**
   * @brief Destructor
   */
  virtual ~AsyncUsbTransceiverBase();

  /**
   * @brief Initialize the transceiver.
   * @returns true if SetupHandle() returned a valid handle, false otherwise.
   */
  bool Init();

  /**
   * @brief Called from the libusb callback when the asynchronous transfer
   *   completes.
   * @param transfer the completed transfer.
   */
  virtual void TransferComplete(struct libusb_transfer *transfer) = 0;

  /**
   * @brief Get the libusb_device_handle of an already opened widget
   * @returns the handle of the widget or NULL if it was not opened
   */
  libusb_device_handle *GetHandle() { return m_usb_handle; }

 protected:
  /**
   * @brief The LibUsbAdaptor passed in the constructor.
   */
  ola::usb::LibUsbAdaptor* const m_adaptor;

  /**
   * @brief The libusb_device passed in the constructor.
   */
  libusb_device* const m_usb_device;

  /**
   * @brief Open the device handle.
   * @returns A valid libusb_device_handle or NULL if the device could not be
   *   opened.
   */
  virtual libusb_device_handle* SetupHandle() = 0;

  /**
   * @brief Called when the transfer completes.
   *
   * Some devices require multiple transfers per DMX frame. This provides a
   * hook for continuation.
   */
  virtual void PostTransferHook() {}

  /**
   * @brief Cancel any pending transfers.
   */
  void CancelTransfer();

  /**
   * @brief Fill a control transfer.
   * @param buffer passed to libusb_fill_control_transfer.
   * @param timeout passed to libusb_fill_control_transfer.
   */
  void FillControlTransfer(unsigned char *buffer, unsigned int timeout);

  /**
   * @brief Fill a bulk transfer.
   */
  void FillBulkTransfer(unsigned char endpoint, unsigned char *buffer,
                        int length, unsigned int timeout);

  /**
   * @brief Fill an interrupt transfer.
   */
  void FillInterruptTransfer(unsigned char endpoint, unsigned char *buffer,
                             int length, unsigned int timeout);

  /**
   * @brief Submit the transfer for tx.
   * @returns the result of libusb_submit_transfer().
   */
  int SubmitTransfer();

  enum TransferState {
    IDLE,
    IN_PROGRESS,
    DISCONNECTED,
  };

  libusb_device_handle *m_usb_handle;
  bool m_suppress_continuation;
  struct libusb_transfer *m_transfer;

  TransferState m_transfer_state;  // GUARDED_BY(m_mutex);
  ola::thread::Mutex m_mutex;

 private:
  DISALLOW_COPY_AND_ASSIGN(AsyncUsbTransceiverBase);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_ASYNCUSBTRANSCEIVERBASE_H_
