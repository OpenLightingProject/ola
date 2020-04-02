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

#include <assert.h>

#include "plugins/usbdmx/AsyncUsbTransceiverBase.h"

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Logging.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::usb::LibUsbAdaptor;

/*
 * Called by libusb when the transfer completes.
 */
#ifdef _WIN32
__attribute__((__stdcall__))
#endif  // _WIN32
void AsyncTransferCallback(struct libusb_transfer *transfer) {
  AsyncUsbTransceiverBase *widget = reinterpret_cast<AsyncUsbTransceiverBase*>(
    transfer->user_data);
  widget->TransferCompleteInternal(transfer);
}

void AsyncUsbTransceiverBase::TransferCompleteInternal(
  struct libusb_transfer *transfer) {
  {
    ola::thread::MutexLocker locker(&m_mutex);
    inflight_set::iterator i = m_inflight.find(transfer);
    if (i == m_inflight.end()) {  // not found
      OLA_WARN << "Mismatched libusb transfer: "
               << transfer << " not in inflight set";
      return;
    }

    m_inflight.erase(i);
    m_idle.push(transfer);
  }

  if (transfer->status == LIBUSB_TRANSFER_NO_DEVICE) {
    m_device_disconnected = true;
  }

  TransferComplete(transfer);
}

AsyncUsbTransceiverBase::AsyncUsbTransceiverBase(LibUsbAdaptor *adaptor,
                                                 libusb_device *usb_device,
                                                 unsigned int num_transfers)
    : m_adaptor(adaptor),
      m_usb_device(usb_device),
      m_usb_handle(NULL),
      m_suppress_continuation(false),
      m_device_disconnected(false) {
  for (unsigned int i=0; i < num_transfers; i++) {
    m_idle.push(m_adaptor->AllocTransfer(0));
  }
  m_adaptor->RefDevice(usb_device);
}

AsyncUsbTransceiverBase::~AsyncUsbTransceiverBase() {
  ola::thread::MutexLocker locker(&m_mutex);

  CancelTransfer();

  m_adaptor->UnrefDevice(m_usb_device);

  while (!m_idle.empty()) {
    m_adaptor->FreeTransfer(m_idle.front());
    m_idle.pop();
  }
}

bool AsyncUsbTransceiverBase::Init() {
  m_usb_handle = SetupHandle();
  return m_usb_handle != NULL;
}

void AsyncUsbTransceiverBase::CancelTransfer() {
  m_suppress_continuation = true;

  inflight_set::iterator i;
  for (i = m_inflight.begin(); i != m_inflight.end(); ++i) {
    int ret = m_adaptor->CancelTransfer(*i);
    /* We demote NOT_FOUND errors to debug here because transfer
     * cancellation naturally races with completion.
     *
     * This error simply means the transfer is not in the "in progress"
     * state anymore.
     *
     * Even if we were to check for this before cancelling it could still
     * happen that the transfer completes between the check and the
     * cancellation taking effect.
     */
    if (ret != 0) {
      log_level level = ret == LIBUSB_ERROR_NOT_FOUND
          ? ola::OLA_LOG_DEBUG : ola::OLA_LOG_WARN;
      OLA_LOG(level)
          << "libusb_cancel_transfer returned "
          << m_adaptor->ErrorCodeToString(ret);
    }
  }
}

struct libusb_transfer *AsyncUsbTransceiverBase::CurrentTransfer() {
  /* This should never happen. In AsyncUsbSender::SendDMX we only ask the
   * superclass to perform a transfer when m_idle is not empty and in
   * AsyncUsbReceiver::Start we kick off the right number of transfers so
   * at completion, when another one would be performed, m_idle will not be
   * empty either. */
  assert(!m_idle.empty());

  return m_idle.front();
}


void AsyncUsbTransceiverBase::FillControlTransfer(unsigned char *buffer,
                                                  unsigned int timeout) {
  m_adaptor->FillControlTransfer(CurrentTransfer(), m_usb_handle, buffer,
                                 &AsyncTransferCallback, this, timeout);
}

void AsyncUsbTransceiverBase::FillBulkTransfer(unsigned char endpoint,
                                               unsigned char *buffer,
                                               int length,
                                               unsigned int timeout) {
  m_adaptor->FillBulkTransfer(CurrentTransfer(), m_usb_handle, endpoint, buffer,
                              length, &AsyncTransferCallback, this, timeout);
}

void AsyncUsbTransceiverBase::FillInterruptTransfer(unsigned char endpoint,
                                                    unsigned char *buffer,
                                                    int length,
                                                    unsigned int timeout) {
  m_adaptor->FillInterruptTransfer(CurrentTransfer(), m_usb_handle, endpoint,
                                   buffer, length, &AsyncTransferCallback,
                                   this, timeout);
}

int AsyncUsbTransceiverBase::SubmitTransfer() {
  struct libusb_transfer *transfer = CurrentTransfer();
  int ret = m_adaptor->SubmitTransfer(transfer);
  if (ret) {
    OLA_WARN << "libusb_submit_transfer returned "
             << m_adaptor->ErrorCodeToString(ret);
    if (ret == LIBUSB_ERROR_NO_DEVICE) {
      m_device_disconnected = true;
    }
    return false;
  }
  m_inflight.insert(transfer);
  m_idle.pop();
  return ret;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
