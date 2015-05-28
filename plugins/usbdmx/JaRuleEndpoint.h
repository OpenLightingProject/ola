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
 * JaRuleEndpoint.h
 * Handles the communication with a Ja Rule USB endpoint.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef PLUGINS_USBDMX_JARULEENDPOINT_H_
#define PLUGINS_USBDMX_JARULEENDPOINT_H_

#include <libusb.h>
#include <ola/io/ByteString.h>
#include <ola/thread/ExecutorInterface.h>
#include <ola/thread/Mutex.h>
#include <ola/util/SequenceNumber.h>

#include <map>
#include <queue>

#include "plugins/usbdmx/LibUsbAdaptor.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Handles communication with a Ja Rule USB Endpoint.
 * This class manages sending commands to the Ja Rule device. It builds the
 * frame and handles the USB transfers required to send the command to the
 * device and retrieve the response.
 *
 * @see https://github.com/OpenLightingProject/ja-rule
 */
class JaRuleEndpoint {
 public:
  typedef enum {
    LOGS_PENDING_FLAG = 0x01,  //!< Log messages are pending
    FLAGS_CHANGED_FLAG = 0x02,  //!< Flags have changed
    MSG_TRUNCATED_FLAG = 0x04  //!< The message has been truncated.
  } StatusFlags;

  /**
   * @brief Indicates the eventual state of a Ja Rule command.
   *
   * Various failures can occur at the libusb layer.
   */
  typedef enum {
    /**
     * @brief The command was sent and a response was received.
     */
    COMMAND_COMPLETED_OK,

    /**
     * @brief The command is malformed.
     *
     * This could mean the payload is too big or a NULL pointer with a non-0
     * size was provided.
     */
     COMMAND_MALFORMED,

    /**
     * @brief An error occured when trying to send the command.
     */
    COMMAND_SEND_ERROR,

    /**
     * @brief The command was not sent as the TX queue was full.
     */
    COMMAND_QUEUE_FULL,

    /**
     * @brief The command was sent but no response was received.
     */
    COMMAND_TIMEOUT,

    /**
     * @brief The command class returned did not match the request.
     */
    COMMAND_CLASS_MISMATCH,

    /**
     * @brief The command was cancelled.
     */
    COMMAND_CANCELLED,
  } CommandResult;

  /**
   * @brief A command completion callback.
   * @tparam The result of the command operation
   * @tparam The return code from the device.
   * @tparam The status flags.
   * @tparam The response payload.
   *
   * If the CommandResult is not COMMAND_COMPLETED_OK, the remaining values are
   * undefined.
   */
  typedef ola::BaseCallback4<
    void, CommandResult, uint8_t, uint8_t, const ola::io::ByteString&>
      CommandCompleteCallback;

  /**
   * @brief The Ja Rule commands.
   */
  typedef enum {
    RESET_DEVICE = 0x00,
    SET_BREAK_TIME = 0x10,
    GET_BREAK_TIME = 0x11,
    SET_MAB_TIME = 0x12,
    GET_MAB_TIME = 0x13,
    SET_RDM_BROADCAST_LISTEN = 0x20,
    GET_RDM_BROADCAST_LISTEN = 0x21,
    SET_RDM_WAIT_TIME = 0x22,
    GET_RDM_WAIT_TIME = 0x23,
    TX_DMX = 0x30,
    RDM_DUB = 0x40,
    RDM_REQUEST = 0x41,
    RDM_BROADCAST_REQUEST = 0x42,
    ECHO_COMMAND = 0xf0,
    GET_LOG = 0xf1,
    GET_FLAGS = 0xf2,
    WRITE_LOG = 0xf3,
  } CommandClass;

  /**
   * @brief Create a new JaRuleEndpoint.
   * @param executor The Executor to run the command complete callbacks on.
   * @param adaptor The LibUsbAdaptor to use.
   * @param device the underlying libusb device. Ownership is not transferred.
   */
  JaRuleEndpoint(ola::thread::ExecutorInterface *executor,
                 AsyncronousLibUsbAdaptor *adaptor,
                 libusb_device *device);

  /**
   * @brief Destructor.
   */
  ~JaRuleEndpoint();

  /**
   * @brief Open the device and claim the USB interface.
   * @returns true if the device was opened and claimed correctly, false
   *   otherwise.
   */
  bool Init();

  /**
   * @brief Cancel all queued and inflight commands.
   * This will immediately run all CommandCompleteCallbacks with the
   * COMMAND_CANCELLED code.
   */
  void CancelAll();

  /**
   * @brief Send a command to the Device.
   * @param command the Command type.
   * @param data the payload data. The data is copied and can be freed once the
   *   method returns.
   * @param size the payload size.
   * @param callback The callback to run when the message operation completes.
   * This may be run immediately in some conditions.
   *
   * SendMessage can be called from any thread, and messages will be queued.
   */
  void SendCommand(CommandClass command, const uint8_t *data, unsigned int size,
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
  enum {
    IN_BUFFER_SIZE = 1024
  };

  enum {
    OUT_BUFFER_SIZE = 1024
  };

  // A command that is in the send queue.
  typedef struct {
    CommandClass command;
    CommandCompleteCallback *callback;
    ola::io::ByteString payload;
  } QueuedCommand;

  // A command that has been sent, and is waiting on a response.
  typedef struct {
    CommandClass command;
    CommandCompleteCallback *callback;
    // TODO(simon): we probably need a counter here to detect timeouts.
  } PendingCommand;

  // The arguments passed to the user supplied callback.
  typedef struct {
     CommandResult result;
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

  void MaybeSendCommand();  // LOCK_REQUIRED(m_mutex);
  bool SubmitInTransfer();  // LOCK_REQUIRED(m_mutex);
  void HandleResponse(const uint8_t *data,
                      unsigned int size);  // LOCK_REQUIRED(m_mutex);

  void ScheduleCallback(CommandCompleteCallback *callback,
                        CommandResult result,
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

  DISALLOW_COPY_AND_ASSIGN(JaRuleEndpoint);
};

}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola

#endif  // PLUGINS_USBDMX_JARULEENDPOINT_H_
