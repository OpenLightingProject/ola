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
 * ThreadedUsbSender.cpp
 * Send DMX data over USB from a dedicated thread.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/ThreadedUsbSender.h"

#include <unistd.h>
#include "ola/Logging.h"

namespace ola {
namespace plugin {
namespace usbdmx {

ThreadedUsbSender::ThreadedUsbSender(libusb_device *usb_device,
                                     libusb_device_handle *usb_handle,
                                     int interface_number)
    : m_term(false),
      m_usb_device(usb_device),
      m_usb_handle(usb_handle),
      m_interface_number(interface_number) {
  libusb_ref_device(usb_device);
}

ThreadedUsbSender::~ThreadedUsbSender() {
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  Join();
  libusb_unref_device(m_usb_device);
}

bool ThreadedUsbSender::Start() {
  bool ret = ola::thread::Thread::Start();
  if (!ret) {
    OLA_WARN << "Failed to start sender thread";
    libusb_release_interface(m_usb_handle, m_interface_number);
    libusb_close(m_usb_handle);
    return false;
  }
  return true;
}

void *ThreadedUsbSender::Run() {
  DmxBuffer buffer;
  if (!m_usb_handle)
    return NULL;

  while (1) {
    {
      ola::thread::MutexLocker locker(&m_term_mutex);
      if (m_term)
        break;
    }

    {
      ola::thread::MutexLocker locker(&m_data_mutex);
      buffer.Set(m_buffer);
    }

    if (buffer.Size()) {
      if (!TransmitBuffer(m_usb_handle, buffer)) {
        OLA_WARN << "Send failed, stopping thread...";
        break;
      }
    } else {
      // sleep for a bit
      usleep(40000);
    }
  }
  libusb_release_interface(m_usb_handle, m_interface_number);
  libusb_close(m_usb_handle);
  return NULL;
}

bool ThreadedUsbSender::SendDMX(const DmxBuffer &buffer) {
  // Store the new data in the shared buffer.
  ola::thread::MutexLocker locker(&m_data_mutex);
  m_buffer.Set(buffer);
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
