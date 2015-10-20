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
 * JaRuleWidgetPort.h
 * A Ja Rule widget port.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef LIBS_USB_JARULEWIDGETPORT_H_
#define LIBS_USB_JARULEWIDGETPORT_H_

#include <libusb.h>
#include <ola/Callback.h>
#include <ola/io/ByteString.h>
#include <ola/thread/ExecutorInterface.h>
#include <ola/thread/Mutex.h>
#include <ola/util/SequenceNumber.h>

#include <map>
#include <queue>

#include "libs/usb/JaRulePortHandle.h"
#include "libs/usb/LibUsbAdaptor.h"
#include "libs/usb/JaRuleWidget.h"

namespace ola {
namespace usb {

/**
 * @brief The internal model of a port on a JaRule device.
 *
 * Each port has its own libusb transfers as well as a command queue. This
 * avoids slow commands on one port blocking another.
 */
class JaRuleWidgetPort {
 public:
  /**
   * @brief Create a new JaRuleWidgetPort.
   * @param executor The Executor to run the callbacks on.
   * @param adaptor The LibUsbAdaptor to use.
   * @param usb_handle the libusb_device_handle for the Ja Rule widget.
   * @param endpoint_number The endpoint to use for transfers.
   * @param uid The device's UID.
   * @param physical_port The physical port index.
   */
  JaRuleWidgetPort(ola::thread::ExecutorInterface *executor,
                   LibUsbAdaptor *adaptor,
                   libusb_device_handle *usb_handle,
                   uint8_t endpoint_number,
                   const ola::rdm::UID &uid,
                   uint8_t physical_port);

  /**
   * @brief Destructor
   */
  ~JaRuleWidgetPort();

  /**
   * @brief Claim the handle to this port.
   * @returns a port handle, ownership is not transferred. Will return NULL if
   *   the port is already claimed.
   */
  JaRulePortHandle* ClaimPort();

  /**
   * @brief Release a handle to a port.
   * @returns a port handle, ownership is not transferred.
   */
  void ReleasePort();

  /**
   * @brief Cancel all commands for this port.
   */
  void CancelAll();

  /**
   * @brief Send a command on this port.
   * @param command the Command type.
   * @param data the payload data. The data is copied and can be freed once the
   *   method returns.
   * @param size the payload size.
   * @param callback The callback to run when the command completes.
   * This may be run immediately in some conditions.
   *
   * SendCommand() can be called from any thread, and messages will be queued.
   */
  void SendCommand(CommandClass command,
                   const uint8_t *data,
                   unsigned int size,
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
  // This must be a multiple of the USB packet size otherwise we can experience
  // overflows. A message can be a maximum of 640 bytes, so we'll use 1k here
  // to be safe.
  enum { IN_BUFFER_SIZE = 1024 };
  enum { OUT_BUFFER_SIZE = 1024 };

  // The arguments passed to the user supplied callback.
  typedef struct {
    USBCommandResult result;
    JaRuleReturnCode return_code;
    uint8_t status_flags;
    const ola::io::ByteString payload;
  } CallbackArgs;

  class PendingCommand {
   public:
    PendingCommand(CommandClass command,
                   CommandCompleteCallback *callback,
                   const ola::io::ByteString &payload)
        : command(command),
          callback(callback),
          payload(payload) {
    }

    CommandClass command;
    CommandCompleteCallback *callback;
    ola::io::ByteString payload;
    TimeStamp out_time;  // When this cmd was sent
  };

  typedef std::map<uint8_t, PendingCommand*> PendingCommandMap;
  typedef std::queue<PendingCommand*> CommandQueue;

  ola::Clock m_clock;
  ola::thread::ExecutorInterface* const m_executor;
  LibUsbAdaptor* const m_adaptor;
  libusb_device_handle* const m_usb_handle;
  const uint8_t m_endpoint_number;
  const ola::rdm::UID m_uid;
  const uint8_t m_physical_port;
  JaRulePortHandle *m_handle;  // NULL if the port isn't claimed

  ola::SequenceNumber<uint8_t> m_token;

  ola::thread::Mutex m_mutex;
  CommandQueue m_queued_commands;  // GUARDED_BY(m_mutex);
  PendingCommandMap m_pending_commands;  // GUARDED_BY(m_mutex);

  libusb_transfer *m_out_transfer;  // GUARDED_BY(m_mutex);
  bool m_out_in_progress;  // GUARDED_BY(m_mutex);

  uint8_t m_in_buffer[IN_BUFFER_SIZE];  // GUARDED_BY(m_mutex);
  libusb_transfer *m_in_transfer;  // GUARDED_BY(m_mutex);
  bool m_in_in_progress;  // GUARDED_BY(m_mutex);

  void MaybeSendCommand();  // LOCK_REQUIRED(m_mutex);
  bool SubmitInTransfer();  // LOCK_REQUIRED(m_mutex);
  void HandleResponse(const uint8_t *data,
                      unsigned int size);  // LOCK_REQUIRED(m_mutex);

  void ScheduleCallback(CommandCompleteCallback *callback,
                        USBCommandResult result,
                        JaRuleReturnCode return_code,
                        uint8_t status_flags,
                        const ola::io::ByteString &payload);
  void RunCallback(CommandCompleteCallback *callback,
                   CallbackArgs args);

  static const uint8_t EOF_IDENTIFIER = 0xa5;
  static const uint8_t SOF_IDENTIFIER = 0x5a;
  static const unsigned int MAX_PAYLOAD_SIZE = 513;
  static const unsigned int MIN_RESPONSE_SIZE = 9;
  static const unsigned int USB_PACKET_SIZE = 64;
  static const unsigned int MAX_IN_FLIGHT = 2;
  static const unsigned int MAX_QUEUED_MESSAGES = 10;

  static const unsigned int ENDPOINT_TIMEOUT_MS = 1000;

  DISALLOW_COPY_AND_ASSIGN(JaRuleWidgetPort);
};
}  // namespace usb
}  // namespace ola
#endif  // LIBS_USB_JARULEWIDGETPORT_H_
