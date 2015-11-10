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
 * AsyncUsbReceiver.h
 * An Asynchronous DMX USB receiver.
 * Copyright (C) 2015 Stefan Krupop
 */

#ifndef PLUGINS_USBDMX_ASYNCUSBRECEIVER_H_
#define PLUGINS_USBDMX_ASYNCUSBRECEIVER_H_

#include <libusb.h>
#include <memory>

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/base/Macro.h"
#include "ola/thread/Mutex.h"
#include "olad/PluginAdaptor.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief A base class that receives DMX data asynchronously from a
 *   libusb_device.
 *
 * This encapsulates much of the asynchronous libusb logic. Subclasses should
 * implement the SetupHandle() and PerformTransfer() methods.
 */
class AsyncUsbReceiver {
 public:
  /**
   * @brief Create a new AsyncUsbReceiver.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   */
  AsyncUsbReceiver(ola::usb::LibUsbAdaptor* const adaptor,
                   libusb_device *usb_device,
                   PluginAdaptor *plugin_adaptor);

  /**
   * @brief Destructor
   */
  virtual ~AsyncUsbReceiver();

  /**
   * @brief Initialize the receiver.
   * @returns true if SetupHandle() returned a valid handle, false otherwise.
   */
  bool Init();

  /**
   * @brief Initialize the receiver with an already setup handle
   * @param handle the handle returned by a previous SetupHandle() call
   * @returns true
   */
  bool Init(libusb_device_handle* handle);

  /**
   * @brief Start receiving DMX
   * @returns the value of PerformTransfer().
   */
  bool Start();

  /**
   * @brief Set the callback to be called when the receive buffer is updated.
   * @param callback The callback to call.
   * @returns NULL.
   */
  void SetReceiveCallback(Callback0<void> *callback) {
    m_receive_callback.reset(callback);
  }

  /**
   * @brief Get DMX Buffer
   * @param buffer DmxBuffer that will get updated with the current input.
   */
  void GetDmx(DmxBuffer *buffer) {
    ola::thread::MutexLocker locker(&m_mutex);
    *buffer = m_rx_buffer;
  }

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
   * @brief Perform the DMX transfer.
   * @returns true if the transfer was scheduled, false otherwise.
   *
   * This method is implemented by the subclass. The subclass should call
   * FillControlTransfer() / FillBulkTransfer() as appropriate and then call
   * SubmitTransfer().
   */
  virtual bool PerformTransfer() = 0;

  /**
   * @brief Called when the transfer completes.
   * @param buffer the DmxBuffer to receive into
   * returns true if the buffer was updated
   */
  virtual bool TransferCompleted(DmxBuffer *buffer) = 0;

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
   * @brief Submit the transfer for rx.
   * @returns the result of libusb_submit_transfer().
   */
  int SubmitTransfer();

 private:
  enum TransferState {
    IDLE,
    IN_PROGRESS,
    DISCONNECTED,
  };

  libusb_device_handle *m_usb_handle;
  PluginAdaptor* const m_plugin_adaptor;
  bool m_inited_with_handle;
  bool m_suppress_continuation;
  struct libusb_transfer *m_transfer;

  TransferState m_transfer_state;  // GUARDED_BY(m_mutex);
  DmxBuffer m_rx_buffer;  // GUARDED_BY(m_mutex);
  ola::thread::Mutex m_mutex;
  std::auto_ptr<Callback0<void> > m_receive_callback;

  DISALLOW_COPY_AND_ASSIGN(AsyncUsbReceiver);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_ASYNCUSBRECEIVER_H_
