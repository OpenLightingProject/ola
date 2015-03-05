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
 * OpenLightingDevice.h
 * The Open Lighting USB Device.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef TOOLS_JA_RULE_OPENLIGHTINGDEVICE_H_
#define TOOLS_JA_RULE_OPENLIGHTINGDEVICE_H_

#include <libusb.h>
#include <queue>

#include <ola/io/SelectServer.h>
#include <ola/thread/Mutex.h>

typedef enum {
  LOGS_PENDING_FLAG = 0x01,  //!< Log messages are pending
  FLAGS_CHANGED_FLAG = 0x02,  //!< Flags have changed
  MSG_TRUNCATED_FLAG = 0x04  //!< The message has been truncated.
} TransportFlags;

/**
 * @brief The interface for message handlers.
 */
class MessageHandlerInterface {
 public:
  struct Message {
    uint16_t command;  //!< The message command
    uint8_t return_code;  //!< The return code.
    uint8_t flags;  //!< The TransportFlags.
    const uint8_t* payload;  //!< A pointer to the payload.
    unsigned int payload_size;  //!< The size of the payload.
  };

  virtual ~MessageHandlerInterface() {}

  /**
   * @brief Handle a new message.
   * @param message The message.
   *
   * The payload data in the message is invalid once the call completes. If you
   * need it to persist the implementation should make a copy.
   */
  virtual void NewMessage(const Message& message) = 0;
};

/**
 * @brief Represents an Open Lighting USB Device.
 */
class OpenLightingDevice {
 public:
  typedef enum {
    ECHO_COMMAND = 0x80,
    TX_DMX = 0x81,
    GET_LOG = 0x82,
    GET_FLAGS = 0x83,
    WRITE_LOG = 0x84,
    RESET_DEVICE = 0x85,
    RDM_DUB = 0x86,
    RDM_REQUEST = 0x87
  } Command;

  /**
   * @brief Create a new OpenLightingDevice.
   * @param ss The SelectServer to execute the Message callbacks on.
   * @param device the underlying libusb device.
   */
  OpenLightingDevice(ola::io::SelectServer* ss, libusb_device* device);

  /**
   * @brief Destructor.
   */
  ~OpenLightingDevice();

  /**
   * @brief Set the message handler.
   * @param handler the MessageHandlerInterface to use. Ownership is not
   * transferred.
   *
   * This should only be called from the same thread the SelectServer is
   * running in.
   */
  void SetHandler(MessageHandlerInterface* handler) {
    m_message_handler = handler;
  }

  /**
   * @brief Initialize and claim the device.
   * @returns true if the device was initialized correctly, false otherwise.
   */
  bool Init();

  /**
   * @brief Send a message to the device.
   * @param command the Command
   * @param data the payload data
   * @param size the payload size.
   * @returns true if the message was sent, false otherwise.
   *
   * SendMessage can be called from any thread, but right now only a single
   * message can be in-flight at once. If the existing message has not
   * completed, further calls to SendMessage will return false.
   */
  bool SendMessage(Command command, const uint8_t *data, unsigned int size);

  /**
   * @brief Called by libusb when the transfer completes or is cancelled.
   */
  void _OutTransferComplete();

  /**
   * @brief Called by libusb when the transfer completes or is cancelled.
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

  ola::io::SelectServer* m_ss;
  libusb_device* m_device;
  libusb_device_handle* m_handle;
  MessageHandlerInterface* m_message_handler;

  ola::thread::Mutex m_transaction_mutex;
  std::queue<PendingRequest> m_queued_requests;  // GUARDED_BY(m_transaction_mutex);
  unsigned int m_in_flight_requests;
  unsigned int m_out_flight_requests;

  ola::thread::Mutex m_out_mutex;
  uint8_t m_out_buffer[OUT_BUFFER_SIZE];  // GUARDED_BY(m_out_mutex);
  libusb_transfer *m_out_transfer;  // GUARDED_BY(m_out_mutex);
  bool m_out_in_progress;  // GUARDED_BY(m_out_mutex);
  ola::TimeStamp m_out_sent_time;

  ola::thread::Mutex m_in_mutex;
  uint8_t m_in_buffer[IN_BUFFER_SIZE];  // GUARDED_BY(m_in_mutex);
  libusb_transfer *m_in_transfer;  // GUARDED_BY(m_in_mutex);
  bool m_in_in_progress;  // GUARDED_BY(m_in_mutex);
  ola::TimeStamp m_send_in_time;

  void MaybeSendRequest();
  bool SubmitInTransfer();

  void HandleData(const uint8_t *data, unsigned int size);

  static const uint8_t EOF_IDENTIFIER = 0xa5;
  static const uint8_t SOF_IDENTIFIER = 0x5a;
  static const unsigned int MAX_PAYLOAD_SIZE = 513;
  static const unsigned int MIN_RESPONSE_SIZE = 8;
  static const unsigned int USB_PACKET_SIZE = 64;
  static const unsigned int MAX_IN_FLIGHT = 2;

  DISALLOW_COPY_AND_ASSIGN(OpenLightingDevice);
};

#endif  // TOOLS_JA_RULE_OPENLIGHTINGDEVICE_H_
