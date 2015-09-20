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
 * JaRuleWidget.cpp
 * A Ja Rule widget.
 * Copyright (C) 2015 Simon Newton
 */

#include "plugins/usbdmx/JaRuleWidget.h"

#include <ola/Callback.h>
#include <ola/Constants.h>
#include <ola/Logging.h>
#include <ola/base/Array.h>
#include <ola/stl/STLUtils.h>
#include <ola/strings/Format.h>
#include <ola/thread/Mutex.h>
#include <ola/util/Utils.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "plugins/usbdmx/JaRulePortHandle.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using jarule::CommandClass;
using jarule::USBCommandResult;
using jarule::JaRulePortHandle;
using ola::NewSingleCallback;
using ola::io::ByteString;
using ola::plugin::usbdmx::LibUsbAdaptor;
using ola::rdm::UID;
using ola::strings::ToHex;
using ola::thread::MutexLocker;
using ola::utils::JoinUInt8;
using ola::utils::SplitUInt16;
using std::auto_ptr;
using std::cout;
using std::string;
using std::vector;

namespace {

#ifdef _WIN32
__attribute__((__stdcall__))
#endif
void InTransferCompleteHandler(struct libusb_transfer *transfer) {
  JaRuleWidget *widget = static_cast<JaRuleWidget*>(
      transfer->user_data);
  return widget->_InTransferComplete();
}

#ifdef _WIN32
__attribute__((__stdcall__))
#endif
void OutTransferCompleteHandler(struct libusb_transfer *transfer) {
  JaRuleWidget *widget = static_cast<JaRuleWidget*>(
      transfer->user_data);
  return widget->_OutTransferComplete();
}
}  // namespace

struct DiscoveredEndpoint {
  DiscoveredEndpoint() : in_supported(false), out_supported(false) {}

  bool in_supported;
  bool out_supported;
};


JaRuleWidget::PortInfo::~PortInfo() {
  delete handle;
}

JaRuleWidget::JaRuleWidget(ola::thread::ExecutorInterface *executor,
                           AsyncronousLibUsbAdaptor *adaptor,
                           libusb_device *usb_device)
  : m_executor(executor),
    m_adaptor(adaptor),
    m_device(usb_device),
    m_usb_handle(NULL),
    m_uid(0, 0),
    m_out_transfer(adaptor->AllocTransfer(0)),
    m_out_in_progress(false),
    m_in_transfer(adaptor->AllocTransfer(0)),
    m_in_in_progress(false) {
  m_adaptor->RefDevice(m_device);
}

JaRuleWidget::~JaRuleWidget() {
  for (unsigned int i = 0; i < m_ports.size(); i++) {
    if (m_ports[i]->claimed) {
      OLA_WARN << "Port " << i << " is still claimed!";
    }
  }

  STLDeleteElements(&m_ports);

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

  if (m_usb_handle) {
    m_adaptor->Close(m_usb_handle);
  }

  m_adaptor->UnrefDevice(m_device);
}

bool JaRuleWidget::Init() {
  bool ok = InternalInit();
  if (!ok) {
    STLDeleteElements(&m_ports);
    if (m_usb_handle) {
      m_adaptor->Close(m_usb_handle);
      m_usb_handle = NULL;
    }
  }
  return ok;
}

void JaRuleWidget::CancelAll(uint8_t port_id) {
  // We don't want to invoke the callbacks while we're holding the lock, so
  // we partition the queued / pending commands into those that match the
  // port_id and those that don't.
  CommandQueue new_queued_commands;
  vector<QueuedCommand> commands_to_cancel;
  vector<PendingCommand> pending_commands_to_cancel;

  {
    MutexLocker locker(&m_mutex);
    // Filter the queued commands into either commands_to_cancel or
    // new_queued_commands.
    while (!m_queued_commands.empty()) {
      QueuedCommand command = m_queued_commands.front();
      m_queued_commands.pop();

      if (command.port_id == port_id) {
        commands_to_cancel.push_back(command);
      } else {
        new_queued_commands.push(command);
      }
    }
    m_queued_commands = new_queued_commands;

    // Now filter the pending commands
    PendingCommandMap::iterator iter = m_pending_commands.begin();
    while (iter != m_pending_commands.end()) {
      if (iter->second.port_id == port_id) {
        pending_commands_to_cancel.push_back(iter->second);
        m_pending_commands.erase(iter++);
      } else {
        ++iter;
      }
    }
  }

  // Cancel all of the queued commands
  vector<QueuedCommand>::iterator iter = commands_to_cancel.begin();
  for (; iter != commands_to_cancel.end(); ++iter) {
    if (iter->callback) {
      iter->callback->Run(jarule::COMMAND_RESULT_TIMEOUT, 0, 0,
                          ByteString());
    }
  }

  // Cancel all of the pending commands
  vector<PendingCommand>::iterator pending_iter =
      pending_commands_to_cancel.begin();
  for (; pending_iter != pending_commands_to_cancel.end(); ++pending_iter) {
    if (pending_iter->callback) {
      pending_iter->callback->Run(jarule::COMMAND_RESULT_TIMEOUT, 0, 0,
                                  ByteString());
    }
  }
}

uint8_t JaRuleWidget::PortCount() const {
  return m_ports.size();
}

UID JaRuleWidget::GetUID() const {
  return m_uid;
}

string JaRuleWidget::ManufacturerString() const {
  return m_manufacturer;
}

string JaRuleWidget::ProductString() const {
  return m_product;
}

JaRulePortHandle* JaRuleWidget::ClaimPort(uint8_t port_index) {
  if (port_index > m_ports.size() - 1) {
    return NULL;
  }
  PortInfo *port_info = m_ports[port_index];
  if (port_info->claimed) {
    return NULL;
  }
  port_info->claimed = true;
  return port_info->handle;
}

void JaRuleWidget::ReleasePort(uint8_t port_index) {
  if (port_index > m_ports.size() - 1) {
    return;
  }
  PortInfo *port_info = m_ports[port_index];
  if (!port_info->claimed) {
    OLA_WARN << "Releasing unclaimed port: " << static_cast<int>(port_index);
  }
  CancelAll(port_index);
  port_info->claimed = false;
}

void JaRuleWidget::SendCommand(uint8_t port_index,
                               CommandClass command,
                               const uint8_t *data,
                               unsigned int size,
                               CommandCompleteCallback *callback) {
  if (port_index > m_ports.size() - 1) {
    OLA_WARN << "Invalid JaRule Port " << static_cast<int>(port_index);
    if (callback) {
      callback->Run(jarule::COMMAND_RESULT_INVALID_PORT, 0, 0, ByteString());
    }
    return;
  }

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
    port_index,
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

void JaRuleWidget::_OutTransferComplete() {
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

void JaRuleWidget::_InTransferComplete() {
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

// Private Methods
// ----------------------------------------------------------------------------

bool JaRuleWidget::InternalInit() {
  struct libusb_config_descriptor *config;
  int error = m_adaptor->GetActiveConfigDescriptor(m_device, &config);
  if (error) {
    OLA_WARN << "Failed to get active config descriptor for Ja Rule device: "
             << m_adaptor->ErrorCodeToString(error);
    return false;
  }

  // Each endpoint address is 8 bits. Bit 7 is the endpoint direction (in/out).
  // The lower 4 bits are the endpoint number. We try to find bulk endpoints
  // with matching numbers.
  typedef std::map<uint8_t, DiscoveredEndpoint> EndpointMap;
  EndpointMap endpoint_map;

  for (uint8_t iface_index = 0; iface_index < config->bNumInterfaces;
       iface_index++) {
    const struct libusb_interface &iface = config->interface[iface_index];
    for (uint8_t alt_setting = 0; alt_setting < iface.num_altsetting;
         alt_setting++) {
      const struct libusb_interface_descriptor &iface_descriptor =
          iface.altsetting[alt_setting];
      if (iface_descriptor.bInterfaceClass == LIBUSB_CLASS_VENDOR_SPEC &&
          iface_descriptor.bInterfaceSubClass == 0xff &&
          iface_descriptor.bInterfaceProtocol == 0xff) {
        // Vendor class, subclass & protocol
        for (uint8_t endpoint_index = 0; endpoint_index <
             iface_descriptor.bNumEndpoints; endpoint_index++) {
          const struct libusb_endpoint_descriptor &endpoint =
              iface_descriptor.endpoint[endpoint_index];
          if ((endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) !=
               LIBUSB_TRANSFER_TYPE_BULK) {
            continue;
          }

          uint8_t endpoint_address = endpoint.bEndpointAddress;
          uint8_t endpoint_number = (
              endpoint_address & LIBUSB_ENDPOINT_ADDRESS_MASK);
          uint8_t endpoint_direction = (
              endpoint_address & LIBUSB_ENDPOINT_DIR_MASK);

          if (endpoint_direction == LIBUSB_ENDPOINT_IN) {
            endpoint_map[endpoint_number].in_supported = true;
          }
          if (endpoint_direction == LIBUSB_ENDPOINT_OUT) {
            endpoint_map[endpoint_number].out_supported = true;
          }
        }
      }
    }
  }

  m_adaptor->FreeConfigDescriptor(config);

  EndpointMap::const_iterator endpoint_iter = endpoint_map.begin();
  uint8_t port_id = 0;
  for (; endpoint_iter != endpoint_map.end(); ++endpoint_iter) {
    if (endpoint_iter->second.in_supported &&
        endpoint_iter->second.out_supported) {
      OLA_INFO << "Found Ja Rule port at "
               << static_cast<int>(endpoint_iter->first);
      m_ports.push_back(
          new PortInfo(endpoint_iter->first,
                       new JaRulePortHandle(this, port_id++)));
    }
  }

  bool ok = m_adaptor->OpenDeviceAndClaimInterface(m_device, INTERFACE_OFFSET,
                                                   &m_usb_handle);
  if (!ok) {
    return false;
  }

  // Get the serial number (UID) of the device.
  libusb_device_descriptor device_descriptor;
  if (m_adaptor->GetDeviceDescriptor(m_device, &device_descriptor)) {
    return false;
  }

  AsyncronousLibUsbAdaptor::DeviceInformation device_info;
  if (!m_adaptor->GetDeviceInfo(m_device, device_descriptor, &device_info)) {
    return false;
  }

  auto_ptr<UID> uid(UID::FromString(device_info.serial));
  if (!uid.get()) {
    OLA_WARN << "Invalid JaRule serial number: " << device_info.serial;
    return false;
  }

  m_uid = *uid;
  m_manufacturer = device_info.manufacturer;
  m_product = device_info.product;

  OLA_INFO << "Found JaRule device : " << m_uid;
  return true;
}

void JaRuleWidget::MaybeSendCommand() {
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

  m_adaptor->FillBulkTransfer(m_out_transfer, m_usb_handle, OUT_ENDPOINT,
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
    command.port_id,
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

bool JaRuleWidget::SubmitInTransfer() {
  if (m_in_in_progress) {
    OLA_WARN << "Read already pending";
    return true;
  }

  m_adaptor->FillBulkTransfer(m_in_transfer, m_usb_handle, IN_ENDPOINT,
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
void JaRuleWidget::HandleResponse(const uint8_t *data, unsigned int size) {
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
void JaRuleWidget::ScheduleCallback(CommandCompleteCallback *callback,
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
    m_executor->Execute(
        NewSingleCallback(this, &JaRuleWidget::RunCallback, callback, args));
  }
}

/*
 * @brief Only ever run in the Executor thread.
 */
void JaRuleWidget::RunCallback(CommandCompleteCallback *callback,
                               CallbackArgs args) {
  callback->Run(args.result, args.return_code, args.status_flags, args.payload);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
