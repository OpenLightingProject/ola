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
 * ThreadedUsbReceiver.cpp
 * Receive DMX data over USB from a dedicated thread.
 * Copyright (C) 2015 Stefan Krupop
 */

#include "plugins/usbdmx/ThreadedUsbReceiver.h"

#include <unistd.h>
#include "ola/Logging.h"

namespace ola {
namespace plugin {
namespace usbdmx {

ThreadedUsbReceiver::ThreadedUsbReceiver(libusb_device *usb_device,
                                         libusb_device_handle *usb_handle,
                                         PluginAdaptor *plugin_adaptor,
                                         int interface_number)
    : m_term(false),
      m_usb_device(usb_device),
      m_usb_handle(usb_handle),
      m_interface_number(interface_number),
      m_plugin_adaptor(plugin_adaptor),
      m_receive_callback(NULL) {
  libusb_ref_device(usb_device);
}

ThreadedUsbReceiver::~ThreadedUsbReceiver() {
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  Join();
  libusb_unref_device(m_usb_device);
}

bool ThreadedUsbReceiver::Start() {
  bool ret = ola::thread::Thread::Start();
  if (!ret) {
    OLA_WARN << "Failed to start receiver thread";
    libusb_release_interface(m_usb_handle, m_interface_number);
    libusb_close(m_usb_handle);
    return false;
  }
  return true;
}

void *ThreadedUsbReceiver::Run() {
  DmxBuffer buffer;
  buffer.Blackout();
  if (!m_usb_handle) {
    return NULL;
  }

  while (1) {
    {
      ola::thread::MutexLocker locker(&m_term_mutex);
      if (m_term) {
        break;
      }
    }

    bool buffer_updated = false;
    if (!ReceiveBuffer(m_usb_handle, &buffer, &buffer_updated)) {
      OLA_WARN << "Receive failed, stopping thread...";
      break;
    }

    if (buffer_updated) {
      {
        ola::thread::MutexLocker locker(&m_data_mutex);
        m_buffer.Set(buffer);
      }
      if (m_receive_callback.get()) {
        m_plugin_adaptor->Execute(m_receive_callback.get());
      }
    }
  }
  libusb_release_interface(m_usb_handle, m_interface_number);
  libusb_close(m_usb_handle);
  return NULL;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
