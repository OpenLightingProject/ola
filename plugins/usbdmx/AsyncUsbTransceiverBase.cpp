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
 * AsyncUsbTransceiverBase.cpp
 * An Asynchronous DMX USB transceiver.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/AsyncUsbTransceiverBase.h"

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
  AsyncUsbTransceiverBase *widget = reinterpret_cast<AsyncUsbTransceiverBase*>(
    transfer->user_data);
  widget->TransferComplete(transfer);
}
}  // namespace

AsyncUsbTransceiverBase::AsyncUsbTransceiverBase(LibUsbAdaptor *adaptor,
                                                 libusb_device *usb_device)
    : m_adaptor(adaptor),
      m_usb_device(usb_device),
      m_usb_handle(NULL),
      m_suppress_continuation(false),
      m_transfer_state(IDLE) {
  m_transfer = m_adaptor->AllocTransfer(0);
  m_adaptor->RefDevice(usb_device);
}

AsyncUsbTransceiverBase::~AsyncUsbTransceiverBase() {
  CancelTransfer();
  m_adaptor->UnrefDevice(m_usb_device);
  m_adaptor->FreeTransfer(m_transfer);
}

bool AsyncUsbTransceiverBase::Init() {
  m_usb_handle = SetupHandle();
  return m_usb_handle != NULL;
}

void AsyncUsbTransceiverBase::CancelTransfer() {
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

void AsyncUsbTransceiverBase::FillControlTransfer(unsigned char *buffer,
                                                  unsigned int timeout) {
  m_adaptor->FillControlTransfer(m_transfer, m_usb_handle, buffer,
                                 &AsyncCallback, this, timeout);
}

void AsyncUsbTransceiverBase::FillBulkTransfer(unsigned char endpoint,
                                               unsigned char *buffer,
                                               int length,
                                               unsigned int timeout) {
  m_adaptor->FillBulkTransfer(m_transfer, m_usb_handle, endpoint, buffer,
                              length, &AsyncCallback, this, timeout);
}

void AsyncUsbTransceiverBase::FillInterruptTransfer(unsigned char endpoint,
                                                    unsigned char *buffer,
                                                    int length,
                                                    unsigned int timeout) {
  m_adaptor->FillInterruptTransfer(m_transfer, m_usb_handle, endpoint, buffer,
                                   length, &AsyncCallback, this, timeout);
}

int AsyncUsbTransceiverBase::SubmitTransfer() {
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
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
