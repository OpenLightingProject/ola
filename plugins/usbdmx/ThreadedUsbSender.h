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
 * ThreadedUsbSender.h
 * Send DMX data over USB from a dedicated thread.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBDMX_THREADEDUSBSENDER_H_
#define PLUGINS_USBDMX_THREADEDUSBSENDER_H_

#include <libusb.h>
#include "ola/base/Macro.h"
#include "ola/DmxBuffer.h"
#include "ola/thread/Thread.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Send DMX data using libusb, from a separate thread.
 *
 * The synchronous libusb calls can sometimes take a while to complete, I've
 * seen cases of up to 21ms.
 * To avoid blocking the main thread, we need to perform the libusb transfer
 * calls in a separate thread. This class contains all the thread management
 * code, leaving the subclass to implement TransmitBuffer(), which performs the
 * actual transfer.
 *
 * ThreadedUsbSender can be used as a building block for synchronous widgets.
 */
class ThreadedUsbSender: private ola::thread::Thread {
 public:
  ThreadedUsbSender(libusb_device *usb_device,
                    libusb_device_handle *usb_handle);
  virtual ~ThreadedUsbSender();

  bool Start();
  void *Run();

  bool SendDMX(const DmxBuffer &buffer);

 protected:
  virtual bool TransmitBuffer(libusb_device_handle *handle,
                              const DmxBuffer &buffer) = 0;

 private:
  bool m_term;
  libusb_device* const m_usb_device;
  libusb_device_handle* const m_usb_handle;
  DmxBuffer m_buffer;
  ola::thread::Mutex m_data_mutex;
  ola::thread::Mutex m_term_mutex;

  DISALLOW_COPY_AND_ASSIGN(ThreadedUsbSender);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_THREADEDUSBSENDER_H_
