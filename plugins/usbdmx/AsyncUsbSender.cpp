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
 * AsyncUsbSender.cpp
 * An Asynchronous DMX USB sender.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/AsyncUsbSender.h"

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Logging.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::usb::LibUsbAdaptor;

AsyncUsbSender::AsyncUsbSender(LibUsbAdaptor *adaptor,
                               libusb_device *usb_device)
    : AsyncUsbTransceiverBase(adaptor, usb_device) {
}

AsyncUsbSender::~AsyncUsbSender() {
  m_adaptor->Close(m_usb_handle);
}

bool AsyncUsbSender::SendDMX(const DmxBuffer &buffer, unsigned int portId) {
  if (!m_usb_handle) {
    OLA_WARN << "AsyncUsbSender hasn't been initialized";
    return false;
  }
  ola::thread::MutexLocker locker(&m_mutex);
  if (m_transfer_state == IDLE) {
    PerformTransfer(buffer, portId);
  } else {
    // Buffer incoming data so we can send it when the outstanding transfers
    // complete.
    if (m_tx_buffers.count(portId)) {
      m_tx_buffers[portId].Set(buffer);
    } else {
      m_tx_buffers.insert(std::pair<unsigned int, DmxBuffer>(portId, buffer));
    }
  }
  return true;
}

void AsyncUsbSender::TransferComplete(struct libusb_transfer *transfer) {
  if (transfer != m_transfer) {
    OLA_WARN << "Mismatched libusb transfer: " << transfer << " != "
             << m_transfer;
    return;
  }

  if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
    OLA_WARN << "Transfer returned " << transfer->status;
  }

  ola::thread::MutexLocker locker(&m_mutex);
  m_transfer_state = (transfer->status == LIBUSB_TRANSFER_NO_DEVICE ?
      DISCONNECTED : IDLE);

  if (m_suppress_continuation) {
    return;
  }

  PostTransferHook();

  if ((m_transfer_state == IDLE) && m_tx_buffers.size()) {
    std::map<unsigned int, DmxBuffer>::iterator it = m_tx_buffers.begin();
    PerformTransfer(it->second, it->first);
    m_tx_buffers.erase(it->first);
  }
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
