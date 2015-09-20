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
 * JaRuleWidgetPort.cpp
 * A Ja Rule Widget Port.
 * Copyright (C) 2015 Simon Newton
 */

#include "plugins/usbdmx/JaRuleWidgetPort.h"

#include <ola/Callback.h>
#include <ola/Constants.h>
#include <ola/Logging.h>
#include <ola/stl/STLUtils.h>
#include <ola/strings/Format.h>
#include <ola/thread/Mutex.h>
#include <ola/util/Utils.h>

#include <string>
#include <utility>

namespace ola {
namespace plugin {
namespace usbdmx {
namespace jarule {

using jarule::CommandClass;
using jarule::USBCommandResult;
using ola::NewSingleCallback;
using ola::io::ByteString;
using ola::plugin::usbdmx::LibUsbAdaptor;
using ola::strings::ToHex;
using ola::thread::MutexLocker;
using ola::utils::JoinUInt8;
using ola::utils::SplitUInt16;
using std::cout;
using std::string;

namespace {

#ifdef _WIN32
__attribute__((__stdcall__))
#endif
void InTransferCompleteHandler(struct libusb_transfer *transfer) {
  JaRuleWidgetPort *port = static_cast<JaRuleWidgetPort*>(transfer->user_data);
  return port->_InTransferComplete();
}

#ifdef _WIN32
__attribute__((__stdcall__))
#endif
void OutTransferCompleteHandler(struct libusb_transfer *transfer) {
  JaRuleWidgetPort *port = static_cast<JaRuleWidgetPort*>(transfer->user_data);
  return port->_OutTransferComplete();
}

}  // namespace

JaRuleWidgetPort::JaRuleWidgetPort(ola::thread::ExecutorInterface *executor,
                   LibUsbAdaptor *adaptor,
                   libusb_device_handle *usb_handle,
                   uint8_t endpoint_number,
                   const ola::rdm::UID &uid,
                   uint8_t physical_port)
    : m_executor(executor),
      m_adaptor(adaptor),
      m_usb_handle(usb_handle),
      m_endpoint_number(endpoint_number),
      m_uid(uid),
      m_physical_port(physical_port),
      m_handle(NULL),
      m_out_transfer(adaptor->AllocTransfer(0)),
      m_out_in_progress(false),
      m_in_transfer(adaptor->AllocTransfer(0)),
      m_in_in_progress(false) {
}

JaRuleWidgetPort::~JaRuleWidgetPort() {
  if (m_handle) {
    OLA_WARN << "JaRulePortHandle is still claimed!";
    delete m_handle;
  }

  {
    MutexLocker locker(&m_mutex);
    if (!m_queued_commands.empty()) {
      OLA_WARN << "Queued commands remain, did we forget to call "
                  "CancelTransfer()?";
    }

    if (!m_pending_commands.empty()) {
      OLA_WARN << "Pending commands remain, did we forget to call "
                  "CancelTransfer()?";
    }

    if (m_out_in_progress) {
      m_adaptor->CancelTransfer(m_out_transfer);
    }

    if (m_in_in_progress) {
      m_adaptor->CancelTransfer(m_in_transfer);
    }
  }

  OLA_DEBUG << "Waiting for transfers to complete";
  bool transfers_pending = true;
  while (transfers_pending) {
    // Spin waiting for the transfers to complete.
    MutexLocker locker(&m_mutex);
    transfers_pending = m_out_in_progress || m_in_in_progress;
  }

  if (m_out_transfer) {
    m_adaptor->FreeTransfer(m_out_transfer);
  }

  if (m_in_transfer) {
    m_adaptor->FreeTransfer(m_in_transfer);
  }
}

JaRulePortHandle* JaRuleWidgetPort::ClaimPort() {
  if (m_handle) {
    return NULL;
  }

  m_handle = new JaRulePortHandle(this, m_uid, m_physical_port);
  return m_handle;
}

void JaRuleWidgetPort::ReleasePort() {
  if (m_handle) {
    delete m_handle;
    m_handle = NULL;
  }
}

void JaRuleWidgetPort::CancelAll() {
  CommandQueue queued_commands;
  PendingCommandMap pending_commands;

  {
    MutexLocker locker(&m_mutex);
    queued_commands = m_queued_commands;
    while (!m_queued_commands.empty()) {
      m_queued_commands.pop();
    }
    pending_commands.swap(m_pending_commands);
  }

  while (!queued_commands.empty()) {
    QueuedCommand queued_command = queued_commands.front();
    if (queued_command.callback) {
      queued_command.callback->Run(jarule::COMMAND_RESULT_TIMEOUT, 0, 0,
                                   ByteString());
    }
    queued_commands.pop();
  }

  PendingCommandMap::iterator iter = pending_commands.begin();
  for (; iter != pending_commands.end(); ++iter) {
    if (iter->second.callback) {
      iter->second.callback->Run(jarule::COMMAND_RESULT_TIMEOUT, 0, 0,
                                 ByteString());
    }
  }

  {
    MutexLocker locker(&m_mutex);
    if (!(m_queued_commands.empty() && m_pending_commands.empty())) {
      OLA_WARN << "Some commands have not been cancelled";
    }
  }
}

void JaRuleWidgetPort::SendCommand(
    jarule::CommandClass command,
    const uint8_t *data,
    unsigned int size,
    CommandCompleteCallback *callback) {
  if (size > MAX_PAYLOAD_SIZE) {
    OLA_WARN << "JaRule message exceeds max payload size";
    if (callback) {
      callback->Run(jarule::COMMAND_RESULT_MALFORMED, 0, 0, ByteString());
    }
    return;
  }

  if (size != 0 && data == NULL) {
    OLA_WARN << "JaRule data is NULL, size was " << size;
    callback->Run(jarule::COMMAND_RESULT_MALFORMED, 0, 0, ByteString());
    return;
  }

  QueuedCommand queued_command = {
    command,
    callback,
    ByteString(data, size)
  };

  MutexLocker locker(&m_mutex);

  OLA_INFO << "Adding new command " << ToHex(command);

  if (m_queued_commands.size() > MAX_QUEUED_MESSAGES) {
    locker.Release();
    OLA_WARN << "JaRule outbound queue is full";
    if (callback) {
      callback->Run(jarule::COMMAND_RESULT_QUEUE_FULL, 0, 0, ByteString());
    }
    return;
  }

  m_queued_commands.push(queued_command);
  MaybeSendCommand();
}

void JaRuleWidgetPort::_OutTransferComplete() {
  OLA_DEBUG << "Out Command status is "
           << LibUsbAdaptor::ErrorCodeToString(m_in_transfer->status);
  if (m_out_transfer->status == LIBUSB_TRANSFER_COMPLETED) {
    if (m_out_transfer->actual_length != m_out_transfer->length) {
      // TODO(simon): decide what to do here
      OLA_WARN << "Only sent " << m_out_transfer->actual_length << " / "
               << m_out_transfer->length << " bytes";
    }
  }

  MutexLocker locker(&m_mutex);
  m_out_in_progress = false;
  MaybeSendCommand();
}

void JaRuleWidgetPort::_InTransferComplete() {
  OLA_DEBUG << "In transfer completed status is "
            << LibUsbAdaptor::ErrorCodeToString(m_in_transfer->status);

  MutexLocker locker(&m_mutex);
  m_in_in_progress = false;

  if (m_in_transfer->status == LIBUSB_TRANSFER_COMPLETED) {
    HandleResponse(m_in_transfer->buffer, m_in_transfer->actual_length);
  }

  // TODO(simon): handle timeouts here
  // Either we'll be getting timouts or we'll be getting good responses from
  // other messages, either way we don't need a RegisterTimeout with the SS.

  if (!m_pending_commands.empty()) {
    SubmitInTransfer();
  }
}

void JaRuleWidgetPort::MaybeSendCommand() {
  if (m_out_in_progress ||
      m_pending_commands.size() > MAX_IN_FLIGHT ||
      m_queued_commands.empty()) {
    return;
  }

  QueuedCommand command = m_queued_commands.front();
  m_queued_commands.pop();

  unsigned int offset = 0;
  uint8_t token = m_token.Next();

  m_out_buffer[0] = SOF_IDENTIFIER;
  m_out_buffer[1] = token;
  SplitUInt16(command.command, &m_out_buffer[3], &m_out_buffer[2]);
  SplitUInt16(command.payload.size(), &m_out_buffer[5], &m_out_buffer[4]);
  offset += 6;

  if (command.payload.size() > 0) {
    memcpy(m_out_buffer + offset, command.payload.data(),
           command.payload.size());
    offset += command.payload.size();
  }
  m_out_buffer[offset++] = EOF_IDENTIFIER;

  if (offset % USB_PACKET_SIZE == 0)  {
    // We need to pad the message so that the transfer completes on the
    // Device side. We could use LIBUSB_TRANSFER_ADD_ZERO_PACKET instead but
    // that isn't avaiable on all platforms.
    m_out_buffer[offset++] = 0;
  }

  m_adaptor->FillBulkTransfer(m_out_transfer, m_usb_handle,
                              m_endpoint_number | LIBUSB_ENDPOINT_OUT,
                              m_out_buffer, offset,
                              OutTransferCompleteHandler,
                              static_cast<void*>(this),
                              ENDPOINT_TIMEOUT_MS);

  int r = m_adaptor->SubmitTransfer(m_out_transfer);
  if (r) {
    OLA_WARN << "Failed to submit outbound transfer: "
             << LibUsbAdaptor::ErrorCodeToString(r);
    ScheduleCallback(command.callback, jarule::COMMAND_RESULT_SEND_ERROR, 0, 0,
                     ByteString());
    return;
  }

  PendingCommand pending_command = {
    command.command,
    command.callback
  };
  std::pair<PendingCommandMap::iterator, bool> p = m_pending_commands.insert(
      PendingCommandMap::value_type(token, pending_command));
  if (!p.second) {
    // We had an old entry, time it out.
    ScheduleCallback(p.first->second.callback, jarule::COMMAND_RESULT_TIMEOUT,
                     0, 0, ByteString());
    p.first->second = pending_command;
  }

  m_out_in_progress = true;
  if (!m_in_in_progress) {
    SubmitInTransfer();
  }
  return;
}

bool JaRuleWidgetPort::SubmitInTransfer() {
  if (m_in_in_progress) {
    OLA_WARN << "Read already pending";
    return true;
  }

  m_adaptor->FillBulkTransfer(m_in_transfer, m_usb_handle,
                              m_endpoint_number | LIBUSB_ENDPOINT_IN,
                              m_in_buffer, IN_BUFFER_SIZE,
                              InTransferCompleteHandler,
                              static_cast<void*>(this),
                              ENDPOINT_TIMEOUT_MS);

  int r = m_adaptor->SubmitTransfer(m_in_transfer);
  if (r) {
    OLA_WARN << "Failed to submit input transfer: "
             << LibUsbAdaptor::ErrorCodeToString(r);
    return false;
  }

  m_in_in_progress = true;
  return true;
}

/*
 *
 */
void JaRuleWidgetPort::HandleResponse(const uint8_t *data, unsigned int size) {
  if (size < MIN_RESPONSE_SIZE) {
    OLA_WARN << "Response was too small, " << size << " bytes, min was "
             << MIN_RESPONSE_SIZE;
    return;
  }

  if (data[0] != SOF_IDENTIFIER) {
    OLA_WARN << "SOF_IDENTIFIER mismatch, was " << ToHex(data[0]);
    return;
  }

  uint8_t token = data[1];
  uint16_t command = JoinUInt8(data[3], data[2]);
  uint16_t payload_size = JoinUInt8(data[5], data[4]);
  uint8_t return_code = data[6];
  uint8_t status_flags = data[7];

  if (payload_size + MIN_RESPONSE_SIZE > size) {
    OLA_WARN << "Message size of " << (payload_size + MIN_RESPONSE_SIZE)
             << " is greater than rx size of " << size;
    return;
  }

  // TODO(simon): Remove this.
  ola::strings::FormatData(&std::cout, data, size);

  if (data[MIN_RESPONSE_SIZE + payload_size - 1] != EOF_IDENTIFIER) {
    OLA_WARN << "EOF_IDENTIFIER mismatch, was "
             << ToHex(data[payload_size + MIN_RESPONSE_SIZE - 1]);
    return;
  }

  PendingCommand pending_request;
  if (!STLLookupAndRemove(&m_pending_commands, token, &pending_request)) {
    return;
  }

  USBCommandResult status = jarule::COMMAND_RESULT_OK;
  if (pending_request.command != command) {
    status = jarule::COMMAND_RESULT_CLASS_MISMATCH;
  }

  ByteString payload;
  if (payload_size) {
    payload.assign(data + MIN_RESPONSE_SIZE - 1, payload_size);
  }
  ScheduleCallback(
      pending_request.callback, status, return_code, status_flags, payload);
}

/*
 * @brief Schedule a callback to be run on the Executor.
 */
void JaRuleWidgetPort::ScheduleCallback(
    CommandCompleteCallback *callback,
    USBCommandResult result,
    uint8_t return_code,
    uint8_t status_flags,
    const ByteString &payload) {
  if (!callback) {
    return;
  }

  CallbackArgs args = {
    result,
    return_code,
    status_flags,
    payload
  };
  if (callback) {
    m_executor->Execute(NewSingleCallback(
          this, &JaRuleWidgetPort::RunCallback, callback, args));
  }
}

/*
 * @brief Only ever run in the Executor thread.
 */
void JaRuleWidgetPort::RunCallback(
    CommandCompleteCallback *callback,
    CallbackArgs args) {
  callback->Run(args.result, args.return_code, args.status_flags, args.payload);
}
}  // namespace jarule
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
