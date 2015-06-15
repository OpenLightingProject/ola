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
 * JaRuleEndpoint.cpp
 * The Open Lighting USB Device.
 * Copyright (C) 2015 Simon Newton
 */

#include "tools/ja-rule/JaRuleEndpoint.h"

#include <libusb.h>

#include <string.h>

#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/Constants.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/base/Array.h>
#include <ola/io/SelectServer.h>
#include <ola/strings/Format.h>
#include <ola/thread/Mutex.h>
#include <ola/util/Utils.h>

#include <memory>
#include <string>

#include "plugins/usbdmx/LibUsbAdaptor.h"

using ola::Clock;
using ola::NewSingleCallback;
using ola::TimeStamp;
using ola::io::SelectServer;
using ola::plugin::usbdmx::LibUsbAdaptor;
using ola::thread::MutexLocker;
using ola::utils::JoinUInt8;
using ola::utils::SplitUInt16;
using std::cout;
using std::string;

namespace {

void InTransferCompleteHandler(struct libusb_transfer *transfer) {
  JaRuleEndpoint *sender = static_cast<JaRuleEndpoint*>(
      transfer->user_data);
  return sender->_InTransferComplete();
}

void OutTransferCompleteHandler(struct libusb_transfer *transfer) {
  JaRuleEndpoint *sender = static_cast<JaRuleEndpoint*>(
      transfer->user_data);
  return sender->_OutTransferComplete();
}
}  // namespace


JaRuleEndpoint::JaRuleEndpoint(SelectServer *ss, libusb_device *device)
  : m_ss(ss),
    m_device(device),
    m_handle(NULL),
    m_message_handler(NULL),
    m_pending_requests(0),
    m_out_transfer(libusb_alloc_transfer(0)),
    m_out_in_progress(false),
    m_token(0),
    m_in_transfer(libusb_alloc_transfer(0)),
    m_in_in_progress(false) {
}

JaRuleEndpoint::~JaRuleEndpoint() {
  {
    MutexLocker locker(&m_mutex);

    if (m_out_in_progress) {
      libusb_cancel_transfer(m_out_transfer);
    }

    if (m_in_in_progress) {
      libusb_cancel_transfer(m_in_transfer);
    }
  }

  OLA_DEBUG << "Waiting for out to complete";
  bool transfers_pending = true;
  while (transfers_pending) {
    // Spin waiting for the transfers to complete.
    MutexLocker locker(&m_mutex);
    transfers_pending = m_out_in_progress || m_in_in_progress;
  }

  if (m_out_transfer) {
    libusb_free_transfer(m_out_transfer);
  }

  if (m_in_transfer) {
    libusb_free_transfer(m_in_transfer);
  }

  if (m_handle) {
    libusb_close(m_handle);
  }
}

bool JaRuleEndpoint::Init() {
  int r = libusb_open(m_device, &m_handle);
  if (r != 0) {
    OLA_WARN << "Failed to open device: "
             << LibUsbAdaptor::ErrorCodeToString(r);
    return false;
  }

  r = libusb_claim_interface(m_handle, INTERFACE_OFFSET);
  if (r) {
    OLA_WARN << "Failed to claim interface: " << INTERFACE_OFFSET;
    libusb_close(m_handle);
    m_handle = NULL;
    return false;
  }
  return true;
}

bool JaRuleEndpoint::SendMessage(Command command,
                                 const uint8_t *data,
                                 unsigned int size) {
  if (size > MAX_PAYLOAD_SIZE) {
    OLA_WARN << "Message exceeds max payload size";
    return false;
  }

  if (size != 0 && data == NULL) {
    OLA_WARN << "Data is NULL";
    return false;
  }

  MutexLocker locker(&m_mutex);
  PendingRequest request = {
    command,
    string(reinterpret_cast<const char*>(data), size)
  };
  m_queued_requests.push(request);
  MaybeSendRequest();
  return true;
}

void JaRuleEndpoint::_OutTransferComplete() {
  TimeStamp now;
  Clock clock;
  clock.CurrentTime(&now);
  OLA_INFO << "Out Command completed in " << (now - m_out_sent_time)
           << ", status is "
           << LibUsbAdaptor::ErrorCodeToString(m_in_transfer->status);
  if (m_out_transfer->status == LIBUSB_TRANSFER_COMPLETED) {
    if (m_out_transfer->actual_length != m_out_transfer->length) {
      OLA_WARN << "Only sent " << m_out_transfer->actual_length << " / "
               << m_out_transfer->length << " bytes";
    }
  }

  MutexLocker locker(&m_mutex);
  m_out_in_progress = false;
  MaybeSendRequest();
}

void JaRuleEndpoint::_InTransferComplete() {
  TimeStamp now;
  Clock clock;
  clock.CurrentTime(&now);
  OLA_INFO << "In transfer completed in " << (now - m_out_sent_time)
           << ", status is "
           << LibUsbAdaptor::ErrorCodeToString(m_in_transfer->status);

  if (m_in_transfer->status == LIBUSB_TRANSFER_COMPLETED) {
    // Ownership of the buffer is transferred to the HandleData method,
    // running on the SS thread.
    m_ss->Execute(
        NewSingleCallback(
          this, &JaRuleEndpoint::HandleData,
          reinterpret_cast<const uint8_t*>(m_in_transfer->buffer),
          static_cast<unsigned int>(m_in_transfer->actual_length)));
  } else {
    delete[] m_in_transfer->buffer;
  }

  MutexLocker locker(&m_mutex);
  m_in_in_progress = false;
  m_pending_requests--;
  if (m_pending_requests) {
    SubmitInTransfer();
  }
}

void JaRuleEndpoint::MaybeSendRequest() {
  if (m_out_in_progress || m_pending_requests > MAX_IN_FLIGHT ||
      m_queued_requests.empty()) {
    return;
  }

  OLA_INFO << "Sending out transfer";
  PendingRequest request = m_queued_requests.front();
  m_queued_requests.pop();

  unsigned int offset = 0;
  m_out_buffer[0] = SOF_IDENTIFIER;
  m_out_buffer[1] = m_token++;
  SplitUInt16(request.command, &m_out_buffer[3], &m_out_buffer[2]);
  SplitUInt16(request.payload.size(), &m_out_buffer[5], &m_out_buffer[4]);
  offset += 6;

  if (request.payload.size() > 0) {
    memcpy(m_out_buffer + offset, request.payload.data(),
           request.payload.size());
    offset += request.payload.size();
  }
  m_out_buffer[offset++] = EOF_IDENTIFIER;

  if (offset % USB_PACKET_SIZE == 0)  {
    // We need to pad the message so that the transfer completes on the
    // Device side. We could use LIBUSB_TRANSFER_ADD_ZERO_PACKET instead but
    // that isn't avaiable on all platforms.
    m_out_buffer[offset++] = 0;
  }

  libusb_fill_bulk_transfer(m_out_transfer, m_handle, OUT_ENDPOINT,
                            m_out_buffer, offset,
                            OutTransferCompleteHandler,
                            static_cast<void*>(this),
                            ENDPOINT_TIMEOUT_MS);

  Clock clock;
  clock.CurrentTime(&m_out_sent_time);
  OLA_INFO << "TX: Sending " << offset << " bytes";

  int r = libusb_submit_transfer(m_out_transfer);

  if (r) {
    OLA_WARN << "Failed to submit out transfer: "
             << LibUsbAdaptor::ErrorCodeToString(r);
    return;
  }

  m_out_in_progress = true;
  m_pending_requests++;
  if (!m_in_in_progress) {
    SubmitInTransfer();
  }
  return;
}

bool JaRuleEndpoint::SubmitInTransfer() {
  if (m_in_in_progress) {
    OLA_WARN << "Read already pending";
    return true;
  }

  uint8_t* rx_buffer = new uint8_t[IN_BUFFER_SIZE];
  libusb_fill_bulk_transfer(m_in_transfer, m_handle, IN_ENDPOINT,
                            rx_buffer, IN_BUFFER_SIZE,
                            InTransferCompleteHandler,
                            static_cast<void*>(this),
                            ENDPOINT_TIMEOUT_MS);

  Clock clock;
  clock.CurrentTime(&m_send_in_time);
  int r = libusb_submit_transfer(m_in_transfer);
  if (r) {
    OLA_WARN << "Failed to submit input transfer: "
             << LibUsbAdaptor::ErrorCodeToString(r);
    return false;
  }

  m_in_in_progress = true;
  return true;
}

void JaRuleEndpoint::HandleData(const uint8_t *data, unsigned int size) {
  ola::ArrayDeleter deleter(data);

  // Right now we assume that the device only sends a single message at a time.
  // If this ever changes from a message model to more of a stream model we'll
  // need to fix this.
  if (!m_message_handler) {
    return;
  }

  if (size < MIN_RESPONSE_SIZE) {
    OLA_WARN << "Response was too small, " << size << " bytes, min was "
             << MIN_RESPONSE_SIZE;
    return;
  }

  if (data[0] != SOF_IDENTIFIER) {
    OLA_WARN << "SOF mismatch, was " << ola::strings::ToHex(data[0]);
    return;
  }

  uint8_t token = data[1];
  uint16_t command = JoinUInt8(data[3], data[2]);
  uint16_t payload_size = JoinUInt8(data[5], data[4]);
  uint8_t return_code = data[6];
  uint8_t flags = data[7];

  if (payload_size + MIN_RESPONSE_SIZE > size) {
    OLA_WARN << "Message size of " << (payload_size + MIN_RESPONSE_SIZE)
             << " is greater than rx size of " << size;
    return;
  }

  // TODO(simon): Remove this.
  ola::strings::FormatData(&std::cout, data, size);

  if (data[MIN_RESPONSE_SIZE + payload_size - 1] != EOF_IDENTIFIER) {
    OLA_WARN << "EOF_IDENTIFIER mismatch, was "
             << ola::strings::ToHex(data[payload_size + MIN_RESPONSE_SIZE - 1]);
    return;
  }

  MessageHandlerInterface::Message message = {
    token, command, return_code, flags,
    data + MIN_RESPONSE_SIZE - 1, payload_size
  };

  m_message_handler->NewMessage(message);
}
