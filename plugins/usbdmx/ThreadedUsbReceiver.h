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
 * ThreadedUsbReceiver.h
 * Receive DMX data over USB from a dedicated thread.
 * Copyright (C) 2015 Simon Newton, Stefan Krupop
 */

#ifndef PLUGINS_USBDMX_THREADEDUSBRECEIVER_H_
#define PLUGINS_USBDMX_THREADEDUSBRECEIVER_H_

#include <libusb.h>
#include <memory>
#include "ola/base/Macro.h"
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/thread/Thread.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Receive DMX data using libusb, from a separate thread.
 *
 * The synchronous libusb calls can sometimes take a while to complete, I've
 * seen cases of up to 21ms.
 *
 * To avoid blocking the main thread, we need to perform the libusb transfer
 * calls in a separate thread. This class contains all the thread management
 * code, leaving the subclass to implement ReceiveBuffer(), which performs the
 * actual transfer.
 *
 * ThreadedUsbReceiver can be used as a building block for synchronous widgets.
 */
class ThreadedUsbReceiver: private ola::thread::Thread {
 public:
  /**
   * @brief Create a new ThreadedUsbReceiver.
   * @param usb_device The usb_device to use. The ThreadedUsbReceiver takes a ref
   *   on the device, while the ThreadedUsbReceiver object exists.
   * @param usb_handle The handle to use for the DMX transfer.
   */
  ThreadedUsbReceiver(libusb_device *usb_device,
                      libusb_device_handle *usb_handle);
  virtual ~ThreadedUsbReceiver();

  /**
   * @brief Start the new thread.
   * @returns true if the thread is running, false otherwise.
   */
  bool Start();

  /**
   * @brief Entry point for the new thread.
   * @returns NULL.
   */
  void *Run();

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
   * @returns DmxBuffer with current input values.
   */
  const DmxBuffer &GetDmxInBuffer() const {
    return m_buffer;
  }

 protected:
  /**
   * @brief Perform the DMX transfer.
   * @param handle the libusb_device_handle to use for the transfer.
   * @param buffer The DmxBuffer to be updated.
   * @param buffer_updated set to true when buffer was updated (=data
       received)
   * @returns true if the transfer was completed, false otherwise.
   *
   * This is called from the sender thread.
   */
  virtual bool ReceiveBuffer(libusb_device_handle *handle,
                             DmxBuffer *buffer,
                             bool *buffer_updated) = 0;

 private:
  bool m_term;
  libusb_device* const m_usb_device;
  libusb_device_handle* const m_usb_handle;
  std::auto_ptr<Callback0<void> > m_receive_callback;
  DmxBuffer m_buffer;
  ola::thread::Mutex m_data_mutex;
  ola::thread::Mutex m_term_mutex;

  DISALLOW_COPY_AND_ASSIGN(ThreadedUsbReceiver);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_THREADEDUSBRECEIVER_H_
