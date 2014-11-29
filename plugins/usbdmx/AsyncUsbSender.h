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
 * AsyncUsbSender.h
 * An Asynchronous DMX USB sender.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_ASYNCUSBSENDER_H_
#define PLUGINS_USBDMX_ASYNCUSBSENDER_H_

#include <libusb.h>

#include "ola/DmxBuffer.h"
#include "ola/base/Macro.h"
#include "ola/thread/Mutex.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief A base class that send DMX data asynchronously to a libusb_device.
 *
 * This encapsulates much of the asynchronous libusb logic. Subclasses should
 * implement the SetupHandle() and PerformTransfer() methods.
 */
class AsyncUsbSender {
 public:
  /**
   * @brief Create a new AsyncUsbSender.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   */
  AsyncUsbSender(class LibUsbAdaptor* const adaptor,
                 libusb_device *usb_device);

  /**
   * @brief Destructor
   */
  virtual ~AsyncUsbSender();

  /**
   * @brief Initialize the sender.
   * @returns true if SetupHandle() returned a valid handle, false otherwise.
   */
  bool Init();

  /**
   * @brief Send one frame of DMX data.
   * @param buffer the DMX data to send.
   * @returns the value of PerformTransfer().
   */
  bool SendDMX(const DmxBuffer &buffer);

  /**
   * @brief Called from the libusb callback when the asynchronous transfer
   *   completes.
   * @param transfer the completed transfer.
   */
  void TransferComplete(struct libusb_transfer *transfer);

 protected:
  /**
   * @brief The LibUsbAdaptor passed in the constructor.
   */
  class LibUsbAdaptor* const m_adaptor;

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
   * @brief Perform the DMX transfer.
   * @param buffer the DMX buffer to send.
   * @returns true if the transfer was scheduled, false otherwise.
   *
   * This method is implemented by the subclass. The subclass should call
   * FillControlTransfer() / FillBulkTransfer() as appropriate and then call
   * SubmitTransfer().
   */
  virtual bool PerformTransfer(const DmxBuffer &buffer) = 0;

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

  /**
   * @brief Check if there is a pending transfer.
   * @returns true if there is a transfer in progress, false otherwise.
   */
  bool TransferPending() const { return m_pending_tx; }

 private:
  enum TransferState {
    IDLE,
    IN_PROGRESS,
    DISCONNECTED,
  };

  libusb_device_handle *m_usb_handle;
  bool m_suppress_continuation;
  struct libusb_transfer *m_transfer;

  TransferState m_transfer_state;  // GUARDED_BY(m_mutex);
  DmxBuffer m_tx_buffer;  // GUARDED_BY(m_mutex);
  bool m_pending_tx;  // GUARDED_BY(m_mutex);
  ola::thread::Mutex m_mutex;

  DISALLOW_COPY_AND_ASSIGN(AsyncUsbSender);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_ASYNCUSBSENDER_H_
