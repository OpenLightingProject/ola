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
 * AsyncUsbReceiver.cpp
 * An Asynchronous DMX USB receiver.
 * Copyright (C) 2015 Stefan Krupop
 */

#include "plugins/usbdmx/AsyncUsbReceiver.h"

#include "ola/Logging.h"

namespace ola {
namespace plugin {
namespace usbdmx {

AsyncUsbReceiver::AsyncUsbReceiver(ola::usb::LibUsbAdaptor *adaptor,
                                   libusb_device *usb_device,
                                   PluginAdaptor *plugin_adaptor)
    : AsyncUsbTransceiverBase(adaptor, usb_device),
      m_plugin_adaptor(plugin_adaptor),
      m_inited_with_handle(false) {
}

AsyncUsbReceiver::~AsyncUsbReceiver() {
  if (!m_inited_with_handle) {
    m_adaptor->Close(m_usb_handle);
  }
}

bool AsyncUsbReceiver::Init() {
  m_usb_handle = SetupHandle();
  m_inited_with_handle = false;
  return m_usb_handle != NULL;
}

bool AsyncUsbReceiver::Init(libusb_device_handle* handle) {
  m_usb_handle = handle;
  m_inited_with_handle = true;
  return true;
}

bool AsyncUsbReceiver::Start() {
  if (!m_usb_handle) {
    OLA_WARN << "AsyncUsbReceiver hasn't been initialized";
    return false;
  }
  ola::thread::MutexLocker locker(&m_mutex);
  return PerformTransfer();
}

void AsyncUsbReceiver::TransferComplete(struct libusb_transfer *transfer) {
  if (transfer != m_transfer) {
    OLA_WARN << "Mismatched libusb transfer: " << transfer << " != "
             << m_transfer;
    return;
  }

  if (transfer->status != LIBUSB_TRANSFER_COMPLETED &&
      transfer->status != LIBUSB_TRANSFER_TIMED_OUT ) {
    OLA_WARN << "Transfer returned " << transfer->status;
  }

  ola::thread::MutexLocker locker(&m_mutex);
  m_transfer_state = (transfer->status == LIBUSB_TRANSFER_NO_DEVICE ?
      DISCONNECTED : IDLE);

  if (m_suppress_continuation) {
    return;
  }

  if (transfer->status != LIBUSB_TRANSFER_TIMED_OUT) {
    if (TransferCompleted(&m_rx_buffer, transfer->actual_length)) {
      // Input changed
      if (m_receive_callback.get()) {
        m_plugin_adaptor->Execute(m_receive_callback.get());
      }
    }
  }

  // Start next request
  PerformTransfer();
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
