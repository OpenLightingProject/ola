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

#include "libs/usb/JaRuleWidgetPort.h"

#include <ola/Callback.h>
#include <ola/Constants.h>
#include <ola/Logging.h>
#include <ola/stl/STLUtils.h>
#include <ola/strings/Format.h>
#include <ola/thread/Mutex.h>
#include <ola/util/Utils.h>

#include <memory>
#include <string>
#include <utility>

namespace ola {
namespace usb {

using ola::NewSingleCallback;
using ola::io::ByteString;
using ola::strings::ToHex;
using ola::thread::MutexLocker;
using ola::utils::JoinUInt8;
using ola::utils::SplitUInt16;
using std::cerr;
using std::string;
using std::auto_ptr;

namespace {

#ifdef _WIN32
__attribute__((__stdcall__))
#endif  // _WIN32
void InTransferCompleteHandler(struct libusb_transfer *transfer) {
  JaRuleWidgetPort *port = static_cast<JaRuleWidgetPort*>(transfer->user_data);
  return port->_InTransferComplete();
}

#ifdef _WIN32
__attribute__((__stdcall__))
#endif  // _WIN32
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

    // Cancelling may take up to a second if the endpoint has stalled. I can't
    // really see a way to speed this up.
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
    auto_ptr<PendingCommand> command(queued_commands.front());
    if (command->callback) {
      command->callback->Run(COMMAND_RESULT_CANCELLED, RC_UNKNOWN, 0,
                             ByteString());
    }
    queued_commands.pop();
  }

  PendingCommandMap::iterator iter = pending_commands.begin();
  for (; iter != pending_commands.end(); ++iter) {
    if (iter->second->callback) {
      iter->second->callback->Run(COMMAND_RESULT_CANCELLED, RC_UNKNOWN, 0,
                                  ByteString());
      delete iter->second;
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
    CommandClass command_class,
    const uint8_t *data,
    unsigned int size,
    CommandCompleteCallback *callback) {
  if (size > MAX_PAYLOAD_SIZE) {
    OLA_WARN << "JaRule message exceeds max payload size";
    if (callback) {
      callback->Run(COMMAND_RESULT_MALFORMED, RC_UNKNOWN, 0, ByteString());
    }
    return;
  }

  if (size != 0 && data == NULL) {
    OLA_WARN << "JaRule data is NULL, size was " << size;
    callback->Run(COMMAND_RESULT_MALFORMED, RC_UNKNOWN, 0, ByteString());
    return;
  }

  // Create the payload
  ByteString payload;
  payload.reserve(size + MIN_RESPONSE_SIZE);

  payload.push_back(SOF_IDENTIFIER);
  payload.push_back(0);  // token, will be set on TX
  payload.push_back(command_class & 0xff);
  payload.push_back(command_class >> 8);
  payload.push_back(size & 0xff);
  payload.push_back(size >> 8);
  payload.append(data, size);
  payload.push_back(EOF_IDENTIFIER);

  if (payload.size() % USB_PACKET_SIZE == 0)  {
    // We need to pad the message so that the transfer completes on the
    // Device side. We could use LIBUSB_TRANSFER_ADD_ZERO_PACKET instead but
    // that isn't available on all platforms.
    payload.push_back(0);
  }

  auto_ptr<PendingCommand> command(new PendingCommand(
      command_class, callback, payload));

  OLA_INFO << "Adding new command " << ToHex(command_class);

  MutexLocker locker(&m_mutex);

  if (m_queued_commands.size() > MAX_QUEUED_MESSAGES) {
    locker.Release();
    OLA_WARN << "JaRule outbound queue is full";
    if (callback) {
      callback->Run(COMMAND_RESULT_QUEUE_FULL, RC_UNKNOWN, 0, ByteString());
    }
    return;
  }

  m_queued_commands.push(command.release());
  MaybeSendCommand();
}

void JaRuleWidgetPort::_OutTransferComplete() {
  OLA_DEBUG << "Out Command status is "
            << LibUsbAdaptor::ErrorCodeToString(m_in_transfer->status);
  if (m_out_transfer->status == LIBUSB_TRANSFER_COMPLETED) {
    if (m_out_transfer->actual_length != m_out_transfer->length) {
      // TODO(simon): Decide what to do here
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

  PendingCommandMap::iterator iter = m_pending_commands.begin();
  TimeStamp time_limit;
  m_clock.CurrentMonotonicTime(&time_limit);
  time_limit -= TimeInterval(1, 0);
  while (iter != m_pending_commands.end()) {
    PendingCommand *command = iter->second;
    if (command->out_time < time_limit) {
      ScheduleCallback(command->callback, COMMAND_RESULT_TIMEOUT, RC_UNKNOWN, 0,
                       ByteString());
      delete command;
      m_pending_commands.erase(iter++);
    } else {
      iter++;
    }
  }

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

  PendingCommand *command = m_queued_commands.front();
  m_queued_commands.pop();

  uint8_t token = m_token.Next();
  command->payload[1] = token;
  m_adaptor->FillBulkTransfer(
      m_out_transfer, m_usb_handle, m_endpoint_number | LIBUSB_ENDPOINT_OUT,
      const_cast<uint8_t*>(command->payload.data()),
      command->payload.size(), OutTransferCompleteHandler,
      static_cast<void*>(this), ENDPOINT_TIMEOUT_MS);

  int r = m_adaptor->SubmitTransfer(m_out_transfer);
  if (r) {
    OLA_WARN << "Failed to submit outbound transfer: "
             << LibUsbAdaptor::ErrorCodeToString(r);
    ScheduleCallback(command->callback, COMMAND_RESULT_SEND_ERROR, RC_UNKNOWN,
                     0, ByteString());
    delete command;
    return;
  }

  m_clock.CurrentMonotonicTime(&command->out_time);
  std::pair<PendingCommandMap::iterator, bool> p = m_pending_commands.insert(
      PendingCommandMap::value_type(token, command));
  if (!p.second) {
    // We had an old entry, cancel it.
    ScheduleCallback(p.first->second->callback,
                     COMMAND_RESULT_CANCELLED, RC_UNKNOWN, 0, ByteString());
    delete p.first->second;
    p.first->second = command;
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
  uint16_t command_class = JoinUInt8(data[3], data[2]);
  uint16_t payload_size = JoinUInt8(data[5], data[4]);

  JaRuleReturnCode return_code = RC_UNKNOWN;
  if (data[6] < RC_LAST) {
    return_code = static_cast<JaRuleReturnCode>(data[6]);
  }
  uint8_t status_flags = data[7];

  if (payload_size + MIN_RESPONSE_SIZE > size) {
    OLA_WARN << "Message size of " << (payload_size + MIN_RESPONSE_SIZE)
             << " is greater than rx size of " << size;
    return;
  }

  // TODO(simon): Remove this.
  if (LogLevel() >= OLA_LOG_INFO) {
    ola::strings::FormatData(&std::cerr, data, size);
  }

  if (data[MIN_RESPONSE_SIZE + payload_size - 1] != EOF_IDENTIFIER) {
    OLA_WARN << "EOF_IDENTIFIER mismatch, was "
             << ToHex(data[payload_size + MIN_RESPONSE_SIZE - 1]);
    return;
  }

  PendingCommand *command;
  if (!STLLookupAndRemove(&m_pending_commands, token, &command)) {
    return;
  }

  USBCommandResult status = COMMAND_RESULT_OK;
  if (command->command != command_class) {
    status = COMMAND_RESULT_CLASS_MISMATCH;
  }

  ByteString payload;
  if (payload_size) {
    payload.assign(data + MIN_RESPONSE_SIZE - 1, payload_size);
  }
  ScheduleCallback(
      command->callback, status, return_code, status_flags, payload);
  delete command;
}

/*
 * @brief Schedule a callback to be run on the Executor.
 */
void JaRuleWidgetPort::ScheduleCallback(
    CommandCompleteCallback *callback,
    USBCommandResult result,
    JaRuleReturnCode return_code,
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
void JaRuleWidgetPort::RunCallback(CommandCompleteCallback *callback,
                                   CallbackArgs args) {
  callback->Run(args.result, args.return_code, args.status_flags, args.payload);
}
}  // namespace usb
}  // namespace ola
