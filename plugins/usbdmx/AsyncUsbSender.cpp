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
 * A Asynchronous DMX USB sender.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/AsyncUsbSender.h"

#include "ola/Logging.h"
#include "plugins/usbdmx/LibUsbAdaptor.h"

namespace ola {
namespace plugin {
namespace usbdmx {

namespace {

/*
 * Called by libusb when the transfer completes.
 */
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
      m_transfer_state(IDLE) {
  m_transfer = libusb_alloc_transfer(0);
  libusb_ref_device(usb_device);
}

AsyncUsbSender::~AsyncUsbSender() {
  CancelTransfer();
  m_adaptor->CloseHandle(m_usb_handle);
  libusb_unref_device(m_usb_device);
}

bool AsyncUsbSender::Init() {
  m_usb_handle = SetupHandle();
  return m_usb_handle ? true : false;
}

bool AsyncUsbSender::SendDMX(const DmxBuffer &buffer) {
  if (!m_usb_handle) {
    OLA_WARN << "AsynchronousAnymaWidget hasn't been initialized";
    return false;
  }
  ola::thread::MutexLocker locker(&m_mutex);
  if (m_transfer_state != IDLE) {
    return true;
  }

  PerformTransfer(buffer);
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
      libusb_cancel_transfer(m_transfer);
      canceled = true;
    }
  }

  libusb_free_transfer(m_transfer);
  m_transfer = NULL;
}

void AsyncUsbSender::FillControlTransfer(unsigned char *buffer,
                                         unsigned int timeout) {
  libusb_fill_control_transfer(
      m_transfer, m_usb_handle, buffer,
      &AsyncCallback, this, timeout);
}

void AsyncUsbSender::FillBulkTransfer(unsigned char endpoint,
                                      unsigned char *buffer,
                                      int length,
                                      unsigned int timeout) {
  libusb_fill_bulk_transfer(
      m_transfer, m_usb_handle, endpoint,
      buffer, length, &AsyncCallback,
      this, timeout);
}

void AsyncUsbSender::FillInterruptTransfer(unsigned char endpoint,
                                           unsigned char *buffer,
                                           int length,
                                           unsigned int timeout) {
  libusb_fill_interrupt_transfer(
      m_transfer, m_usb_handle, endpoint,
      buffer, length, &AsyncCallback,
      this, timeout);
}

int AsyncUsbSender::SubmitTransfer() {
  int ret = libusb_submit_transfer(m_transfer);
  if (ret) {
    OLA_WARN << "libusb_submit_transfer returned " << libusb_error_name(ret);
    if (ret == LIBUSB_ERROR_NO_DEVICE) {
      m_transfer_state = DISCONNECTED;
    }
    return false;
  }
  m_transfer_state = IN_PROGRESS;
  return ret;
}

void AsyncUsbSender::TransferComplete(
    struct libusb_transfer *transfer) {
  if (transfer != m_transfer) {
    OLA_WARN << "Mismatched libusb transfer: " << transfer << " != "
             << m_transfer;
    return;
  }

  if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
    OLA_WARN << "Transfer returned " << transfer->status;
  }

  ola::thread::MutexLocker locker(&m_mutex);
  m_transfer_state = transfer->status == LIBUSB_TRANSFER_NO_DEVICE ?
      DISCONNECTED : IDLE;
  PostTransferHook();
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
