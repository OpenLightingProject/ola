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

namespace {

/*
 * Called by libusb when the transfer completes.
 */
#ifdef _WIN32
__attribute__((__stdcall__))
#endif
void AsyncCallback(struct libusb_transfer *transfer) {
  AsyncUsbSender *widget = reinterpret_cast<AsyncUsbSender*>(
      transfer->user_data);
  widget->TransferComplete(transfer);
}
}  // namespace

AsyncUsbSender::AsyncUsbSender(LibUsbAdaptor *adaptor,
                               libusb_device *usb_device)
    : m_adaptor(adaptor),
      m_usb_device(usb_device),
      m_usb_handle(NULL),
      m_suppress_continuation(false),
      m_transfer_state(IDLE),
      m_pending_tx(false) {
  m_transfer = m_adaptor->AllocTransfer(0);
  m_adaptor->RefDevice(usb_device);
}

AsyncUsbSender::~AsyncUsbSender() {
  CancelTransfer();
  m_adaptor->Close(m_usb_handle);
  m_adaptor->UnrefDevice(m_usb_device);
  m_adaptor->FreeTransfer(m_transfer);
}

bool AsyncUsbSender::Init() {
  m_usb_handle = SetupHandle();
  return m_usb_handle ? true : false;
}

bool AsyncUsbSender::SendDMX(const DmxBuffer &buffer) {
  if (!m_usb_handle) {
    OLA_WARN << "AsyncUsbSender hasn't been initialized";
    return false;
  }
  ola::thread::MutexLocker locker(&m_mutex);
  if (m_transfer_state == IDLE) {
    PerformTransfer(buffer);
  } else {
    // Buffer incoming data so we can send it when the outstanding transfers
    // complete.
    m_pending_tx = true;
    m_tx_buffer.Set(buffer);
  }
  return true;
}

void AsyncUsbSender::CancelTransfer() {
  if (!m_transfer) {
    return;
  }

  bool canceled = false;
  while (1) {
    ola::thread::MutexLocker locker(&m_mutex);
    if (m_transfer_state == IDLE || m_transfer_state == DISCONNECTED) {
      break;
    }
    if (!canceled) {
      m_suppress_continuation = true;
      if (m_adaptor->CancelTransfer(m_transfer) == 0) {
        canceled = true;
      } else {
        break;
      }
    }
  }

  m_suppress_continuation = false;
}

void AsyncUsbSender::FillControlTransfer(unsigned char *buffer,
                                         unsigned int timeout) {
  m_adaptor->FillControlTransfer(m_transfer, m_usb_handle, buffer,
                                 &AsyncCallback, this, timeout);
}

void AsyncUsbSender::FillBulkTransfer(unsigned char endpoint,
                                      unsigned char *buffer,
                                      int length,
                                      unsigned int timeout) {
  m_adaptor->FillBulkTransfer(m_transfer, m_usb_handle, endpoint, buffer,
                              length, &AsyncCallback, this, timeout);
}

void AsyncUsbSender::FillInterruptTransfer(unsigned char endpoint,
                                           unsigned char *buffer,
                                           int length,
                                           unsigned int timeout) {
  m_adaptor->FillInterruptTransfer(m_transfer, m_usb_handle, endpoint, buffer,
                                   length, &AsyncCallback, this, timeout);
}

int AsyncUsbSender::SubmitTransfer() {
  int ret = m_adaptor->SubmitTransfer(m_transfer);
  if (ret) {
    OLA_WARN << "libusb_submit_transfer returned "
             << m_adaptor->ErrorCodeToString(ret);
    if (ret == LIBUSB_ERROR_NO_DEVICE) {
      m_transfer_state = DISCONNECTED;
    }
    return false;
  }
  m_transfer_state = IN_PROGRESS;
  return ret;
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

  if ((m_transfer_state == IDLE) && m_pending_tx) {
    m_pending_tx = false;
    PerformTransfer(m_tx_buffer);
  }
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
