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
 * JaRuleWidget.h
 * A Ja Rule widget.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef PLUGINS_USBDMX_JARULEWIDGET_H_
#define PLUGINS_USBDMX_JARULEWIDGET_H_

#include <libusb.h>
#include <stdint.h>

#include <ola/io/ByteString.h>
#include <ola/rdm/UID.h>
#include <ola/thread/ExecutorInterface.h>
#include <ola/thread/Mutex.h>
#include <ola/util/SequenceNumber.h>

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "plugins/usbdmx/LibUsbAdaptor.h"
#include "plugins/usbdmx/JaRuleConstants.h"
#include "plugins/usbdmx/Widget.h"

namespace ola {
namespace plugin {
namespace usbdmx {

namespace jarule {
class JaRulePortHandle;
}  // namespace jarule

/**
 * @brief A Ja Rule hardware device (widget).
 *
 * Ja Rule devices may have more than one DMX/RDM port.
 *
 * There are two methods of communicating with a Ja Rule device. Both use this
 * class.
 *
 * The low level request / response method is to call SendCommand() and pass in
 * the callback to run when the command completes.
 *
 * The high level API is to use a JaRulePortHandle, which represents a
 * specific 485 port on the device. To obtain a JaRulePortHandle, call
 * ClaimPort(), when you're finished with the JaRulePortHandle you must call
 * ReleasePort().
 *
 * Calls using the two APIs should not be mixed.
 */
class JaRuleWidget : public WidgetInterface {
 public:
  /**
   * @brief A command completion callback.
   * @tparam The result of the command operation
   * @tparam The return code from the device.
   * @tparam The status flags.
   * @tparam The response payload.
   *
   * If the USBCommandResult is not COMMAND_COMPLETED_OK, the remaining values
   * are undefined.
   */
  typedef ola::BaseCallback4<void, jarule::USBCommandResult, uint8_t, uint8_t,
                             const ola::io::ByteString&>
                               CommandCompleteCallback;

  /**
   * @brief Create a new Ja Rule widget.
   * @param executor The Executor to run the callbacks on.
   * @param adaptor The LibUsbAdaptor to use.
   * @param usb_device the libusb_device for the Ja Rule widget.
   */
  JaRuleWidget(ola::thread::ExecutorInterface *executor,
               AsyncronousLibUsbAdaptor *adaptor,
               libusb_device *usb_device);

  /**
   * @brief Destructor
   */
  ~JaRuleWidget();

  /**
   * @brief Initialize the Ja Rule widget.
   * @returns true if the USB device was opened and claimed correctly, false
   *   otherwise.
   */
  bool Init();

  /**
   * @brief Cancel all queued and inflight commands.
   * @param port_id The port id of the commands to cancel
   *
   * This will immediately run all CommandCompleteCallbacks with the
   * COMMAND_CANCELLED code.
   */
  void CancelAll(uint8_t port_id);

  /**
   * @brief The number of ports on the widget, valid after Init() has returned
   *   true.
   * @returns The number of ports.
   *
   * Ports are numbered consecutively from 0.
   */
  uint8_t PortCount() const;

  /**
   * @brief The UID of the widget, valid after Init() has returned true.
   * @returns The UID for the device.
   */
  ola::rdm::UID GetUID() const;

  /**
   * @brief Get the manufacturer string.
   * @returns The manufacturer string.
   *
   * The return value is only valid after Init() has returned.
   */
  std::string ManufacturerString() const;

  /**
   * @brief Get the product string.
   * @returns The product string.
   *
   * The return value is only valid after Init() has returned.
   */
  std::string ProductString() const;

  /**
   * @brief Claim a handle to a port.
   * @param port_index The port to claim.
   * @returns a port handle, ownership is not transferred. Will return NULL if
   *   the port id is invalid, or already claimed.
   */
  jarule::JaRulePortHandle* ClaimPort(uint8_t port_index);

  /**
   * @brief Release a handle to a port.
   * @param port_index The port to claim
   * @returns a port handle, ownership is not transferred.
   */
  void ReleasePort(uint8_t port_index);

  /**
   * @brief The low level method to send a command to the widget.
   * @param port_index The port on which to send the command.
   * @param command the Command type.
   * @param data the payload data. The data is copied and can be freed once the
   *   method returns.
   * @param size the payload size.
   * @param callback The callback to run when the message operation completes.
   * This may be run immediately in some conditions.
   *
   * SendMessage can be called from any thread, and messages will be queued.
   */
  void SendCommand(uint8_t port_index, jarule::CommandClass command,
                   const uint8_t *data, unsigned int size,
                   CommandCompleteCallback *callback);

  /**
   * @brief Called by the libusb callback when the transfer completes or is
   * cancelled.
   */
  void _OutTransferComplete();

  /**
   * @brief Called by the libusb callback when the transfer completes or is
   * cancelled.
   */
  void _InTransferComplete();

 private:
  // TODO(simon): We should move to a model where we have separate queue for
  // each port, this will give us isolation and avoid problems like #155
  struct PortInfo {
    PortInfo(uint8_t endpoint_number, jarule::JaRulePortHandle *handle)
        : claimed(false), endpoint_number(endpoint_number), handle(handle) {}
    ~PortInfo();

    bool claimed;
    uint8_t endpoint_number;
    jarule::JaRulePortHandle *handle;
  };

  typedef std::vector<PortInfo*> PortHandles;

  // This must be a multiple of the USB packet size otherwise we can experience
  // overflows. A message can be a maximum of 640 bytes, so we'll use 1k here
  // to be safe.
  enum { IN_BUFFER_SIZE = 1024 };
  enum { OUT_BUFFER_SIZE = 1024 };

  // A command that is in the send queue.
  typedef struct {
    uint8_t port_id;
    jarule::CommandClass command;
    CommandCompleteCallback *callback;
    ola::io::ByteString payload;
  } QueuedCommand;

  // A command that has been sent, and is waiting on a response.
  typedef struct {
    uint8_t port_id;
    jarule::CommandClass command;
    CommandCompleteCallback *callback;
    // TODO(simon): we probably need a counter here to detect timeouts.
  } PendingCommand;

  // The arguments passed to the user supplied callback.
  typedef struct {
    jarule::USBCommandResult result;
     uint8_t return_code;
     uint8_t status_flags;
     const ola::io::ByteString payload;
  } CallbackArgs;

  typedef std::map<uint8_t, PendingCommand> PendingCommandMap;
  typedef std::queue<QueuedCommand> CommandQueue;

  ola::thread::ExecutorInterface *m_executor;
  LibUsbAdaptor *m_adaptor;
  libusb_device *m_device;
  libusb_device_handle *m_usb_handle;
  ola::rdm::UID m_uid;  // The UID of the device, or 0000:00000000 if unset
  std::string m_manufacturer;
  std::string m_product;
  PortHandles m_ports;  // The list of port handles.

  ola::SequenceNumber<uint8_t> m_token;

  ola::thread::Mutex m_mutex;
  CommandQueue m_queued_commands;  // GUARDED_BY(m_mutex);
  PendingCommandMap m_pending_commands;  // GUARDED_BY(m_mutex);

  uint8_t m_out_buffer[OUT_BUFFER_SIZE];  // GUARDED_BY(m_mutex);
  libusb_transfer *m_out_transfer;  // GUARDED_BY(m_mutex);
  bool m_out_in_progress;  // GUARDED_BY(m_mutex);

  uint8_t m_in_buffer[IN_BUFFER_SIZE];  // GUARDED_BY(m_mutex);
  libusb_transfer *m_in_transfer;  // GUARDED_BY(m_mutex);
  bool m_in_in_progress;  // GUARDED_BY(m_mutex);

  bool InternalInit();

  void MaybeSendCommand();  // LOCK_REQUIRED(m_mutex);
  bool SubmitInTransfer();  // LOCK_REQUIRED(m_mutex);
  void HandleResponse(const uint8_t *data,
                      unsigned int size);  // LOCK_REQUIRED(m_mutex);

  void ScheduleCallback(CommandCompleteCallback *callback,
                        jarule::USBCommandResult result,
                        uint8_t return_code,
                        uint8_t status_flags,
                        const ola::io::ByteString &payload);
  void RunCallback(CommandCompleteCallback *callback, CallbackArgs args);

  static const uint8_t EOF_IDENTIFIER = 0xa5;
  static const uint8_t SOF_IDENTIFIER = 0x5a;
  static const unsigned int MAX_PAYLOAD_SIZE = 513;
  static const unsigned int MIN_RESPONSE_SIZE = 9;
  static const unsigned int USB_PACKET_SIZE = 64;
  static const unsigned int MAX_IN_FLIGHT = 2;
  static const unsigned int MAX_QUEUED_MESSAGES = 10;
  static const unsigned int INTERFACE_OFFSET = 2;

  static const uint8_t IN_ENDPOINT = 0x81;
  static const uint8_t OUT_ENDPOINT = 0x01;
  static const unsigned int ENDPOINT_TIMEOUT_MS = 1000;

  DISALLOW_COPY_AND_ASSIGN(JaRuleWidget);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_JARULEWIDGET_H_
