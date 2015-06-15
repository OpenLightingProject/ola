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

#ifndef TOOLS_JA_RULE_JARULEENDPOINT_H_
#define TOOLS_JA_RULE_JARULEENDPOINT_H_

#include <libusb.h>
#include <ola/io/SelectServer.h>
#include <ola/thread/Mutex.h>

#include <queue>
#include <string>

typedef enum {
  LOGS_PENDING_FLAG = 0x01,  //!< Log messages are pending
  FLAGS_CHANGED_FLAG = 0x02,  //!< Flags have changed
  MSG_TRUNCATED_FLAG = 0x04  //!< The message has been truncated.
} TransportFlags;

/**
 * @brief Handles communication with a Ja Rule USB Endpoint.
 *
 * @see https://github.com/OpenLightingProject/ja-rule
 */
class JaRuleEndpoint {
 public:
  /**
   * @brief The various Ja Rule commands
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
    WRITE_LOG = 0xf3
  } Command;

  /**
   * @brief The interface for Ja Rule message handlers.
   */
  class MessageHandlerInterface {
   public:
    struct Message {
      uint8_t token;  //!< The message token
      uint16_t command;  //!< The message command
      uint8_t return_code;  //!< The return code.
      uint8_t flags;  //!< The TransportFlags.
      const uint8_t *payload;  //!< A pointer to the payload.
      unsigned int payload_size;  //!< The size of the payload.
    };

    virtual ~MessageHandlerInterface() {}

    /**
     * @brief Handle a new message.
     * @param message The message.
     *
     * The payload data in the message is invalid once the call completes. If
     * you need it to persist the implementation should make a copy.
     */
    virtual void NewMessage(const Message &message) = 0;
  };

  /**
   * @brief Create a new JaRuleEndpoint.
   * @param ss The SelectServer to execute the message receive callbacks on.
   * @param device the underlying libusb device. Ownership is not transferred.
   */
  JaRuleEndpoint(ola::io::SelectServer *ss, libusb_device *device);

  /**
   * @brief Destructor.
   */
  ~JaRuleEndpoint();

  /**
   * @brief Set the message handler.
   * @param handler the JaRuleEndpoint::MessageHandlerInterface to use.
   *   Ownership is not transferred.
   *
   * This should only be called from the same thread the SelectServer is
   * running in.
   */
  void SetHandler(MessageHandlerInterface *handler) {
    m_message_handler = handler;
  }

  /**
   * @brief Open the device and claim the USB interface.
   * @returns true if the device was opened and claimed correctly, false
   *   otherwise.
   */
  bool Init();

  /**
   * @brief Send a message to the endpoint.
   * @param command the Command type.
   * @param data the payload data. The data is copied and can be freed once the
   *   method returns.
   * @param size the payload size.
   * @returns false if the message was malformed, true if it was queued
   *   correctly.
   *
   * SendMessage can be called from any thread, and messages will be queued.
   */
  bool SendMessage(Command command, const uint8_t *data, unsigned int size);

  /**
   * @brief Called by the libusb callback when the outbound transfer completes
   * or is cancelled.
   */
  void _OutTransferComplete();

  /**
   * @brief Called by the libusb callback when the inbound transfer completes
   * or is cancelled.
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

  typedef struct {
    Command command;
    std::string payload;
  } PendingRequest;

  ola::io::SelectServer *m_ss;
  libusb_device *m_device;
  libusb_device_handle *m_handle;
  MessageHandlerInterface *m_message_handler;

  ola::thread::Mutex m_mutex;
  std::queue<PendingRequest> m_queued_requests;  // GUARDED_BY(m_mutex);
  // The number of request frames we've already sent to the device. We limit
  // the number of outstanding requests to MAX_IN_FLIGHT.
  unsigned int m_pending_requests;

  uint8_t m_out_buffer[OUT_BUFFER_SIZE];  // GUARDED_BY(m_mutex);
  libusb_transfer *m_out_transfer;  // GUARDED_BY(m_mutex);
  bool m_out_in_progress;  // GUARDED_BY(m_mutex);
  ola::TimeStamp m_out_sent_time;
  uint8_t m_token;

  uint8_t m_in_buffer[IN_BUFFER_SIZE];  // GUARDED_BY(m_mutex);
  libusb_transfer *m_in_transfer;  // GUARDED_BY(m_mutex);
  bool m_in_in_progress;  // GUARDED_BY(m_mutex);
  ola::TimeStamp m_send_in_time;

  void MaybeSendRequest();  // LOCK_REQUIRED(m_mutex);
  bool SubmitInTransfer();  // LOCK_REQUIRED(m_mutex);

  void HandleData(const uint8_t *data, unsigned int size);

  static const uint8_t EOF_IDENTIFIER = 0xa5;
  static const uint8_t SOF_IDENTIFIER = 0x5a;
  static const unsigned int MAX_PAYLOAD_SIZE = 513;
  static const unsigned int MIN_RESPONSE_SIZE = 9;
  static const unsigned int USB_PACKET_SIZE = 64;
  static const unsigned int MAX_IN_FLIGHT = 2;
  static const unsigned int INTERFACE_OFFSET = 2;

  static const uint8_t IN_ENDPOINT = 0x81;
  static const uint8_t OUT_ENDPOINT = 0x01;
  static const unsigned int ENDPOINT_TIMEOUT_MS = 1000;

  DISALLOW_COPY_AND_ASSIGN(JaRuleEndpoint);
};

#endif  // TOOLS_JA_RULE_JARULEENDPOINT_H_
