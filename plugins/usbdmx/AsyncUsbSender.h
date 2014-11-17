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
 * 
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
 * @brief 
 */
class AsyncUsbSender {
 public:
  /**
   * @brief Create a new AsynchronousAnymaWidget.
   * @param usb_device the libusb_device to use for the widget.
   */
  AsyncUsbSender(class LibUsbAdaptor* const adaptor,
                 libusb_device *usb_device);

  virtual ~AsyncUsbSender();

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

  /**
   * @brief Called from the libusb callback when the asynchronous transfer
   *   completes.
   * @param transfer the completed transfer.
   */
  void TransferComplete(struct libusb_transfer *transfer);

 protected:
  class LibUsbAdaptor* const m_adaptor;
  libusb_device* const m_usb_device;

  virtual libusb_device_handle* SetupHandle() = 0;

  virtual void PerformTransfer(const DmxBuffer &buffer) = 0;

  void CancelTransfer();
  void FillControlTransfer(unsigned char *buffer, unsigned int timeout);
  void FillBulkTransfer(unsigned char endpoint, unsigned char *buffer,
                        int length, unsigned int timeout);

  int SubmitTransfer();

 private:
  enum TransferState {
    IDLE,
    IN_PROGRESS,
    DISCONNECTED,
  };

  libusb_device_handle *m_usb_handle;
  struct libusb_transfer *m_transfer;

  TransferState m_transfer_state;  // GUARDED_BY(m_mutex);
  ola::thread::Mutex m_mutex;

  DISALLOW_COPY_AND_ASSIGN(AsyncUsbSender);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_ASYNCUSBSENDER_H_
