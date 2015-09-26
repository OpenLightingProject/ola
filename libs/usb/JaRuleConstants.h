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
 * JaRuleConstants.h
 * Constants used with Ja Rule devices.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef LIBS_USB_JARULECONSTANTS_H_
#define LIBS_USB_JARULECONSTANTS_H_

#include <ola/Callback.h>

namespace ola {
namespace usb {

/**
 * @brief Ja Rule status flags.
 */
typedef enum {
  LOGS_PENDING_FLAG = 0x01,  //!< Log messages are pending
  FLAGS_CHANGED_FLAG = 0x02,  //!< Flags have changed
  MSG_TRUNCATED_FLAG = 0x04  //!< The message has been truncated.
} StatusFlags;

/**
 * @brief Ja Rule Port modes.
 */
typedef enum {
  CONTROLLER_MODE,  //!< DMX/RDM Controller mode
  RESPONDER_MODE,  //!< DMX/RDM Responder mode
} PortMode;

/**
 * @brief Indicates the eventual state of a Ja Rule command.
 *
 * Various failures can occur at the libusb layer.
 */
typedef enum {
  /**
   * @brief The command was sent and a response was received.
   */
  COMMAND_RESULT_OK,

  /**
   * @brief The command is malformed.
   *
   * This could mean the payload is too big or a NULL pointer with a non-0
   * size was provided.
   */
  COMMAND_RESULT_MALFORMED,

  /**
   * @brief An error occured when trying to send the command.
   */
  COMMAND_RESULT_SEND_ERROR,

  /**
   * @brief The command was not sent as the TX queue was full.
   */
  COMMAND_RESULT_QUEUE_FULL,

  /**
   * @brief The command was sent but no response was received.
   */
  COMMAND_RESULT_TIMEOUT,

  /**
   * @brief The command class returned did not match the request.
   */
  COMMAND_RESULT_CLASS_MISMATCH,

  /**
   * @brief The command was cancelled.
   */
  COMMAND_RESULT_CANCELLED,

  /**
   * @brief Invalid port
   */
  COMMAND_RESULT_INVALID_PORT
} USBCommandResult;

/**
 * @brief The Ja Rule command set.
 */
typedef enum {
  JARULE_CMD_RESET_DEVICE = 0x00,
  JARULE_CMD_SET_MODE = 0x01,
  JARULE_CMD_GET_UID = 0x02,
  JARULE_CMD_SET_BREAK_TIME = 0x10,
  JARULE_CMD_GET_BREAK_TIME = 0x11,
  JARULE_CMD_SET_MARK_TIME = 0x12,
  JARULE_CMD_GET_MARK_TIME = 0x13,
  JARULE_CMD_SET_RDM_BROADCAST_TIMEOUT = 0x20,
  JARULE_CMD_GET_RDM_BROADCAST_TIMEOUT = 0x21,
  JARULE_CMD_SET_RDM_RESPONSE_TIMEOUT = 0x22,
  JARULE_CMD_GET_RDM_RESPONSE_TIMEOUT = 0x23,
  JARULE_CMD_SET_RDM_DUB_RESPONSE_LIMIT = 0x24,
  JARULE_CMD_GET_RDM_DUB_RESPONSE_LIMIT = 0x25,
  JARULE_CMD_SET_RDM_RESPONDER_DELAY = 0x26,
  JARULE_CMD_GET_RDM_RESPONDER_DELAY = 0x27,
  JARULE_CMD_SET_RDM_RESPONDER_JITTER = 0x28,
  JARULE_CMD_GET_RDM_RESPONDER_JITTER = 0x29,
  JARULE_CMD_TX_DMX = 0x30,
  JARULE_CMD_RDM_DUB_REQUEST = 0x40,
  JARULE_CMD_RDM_REQUEST = 0x41,
  JARULE_CMD_RDM_BROADCAST_REQUEST = 0x42,

  // Experimental / testing
  JARULE_CMD_ECHO = 0xf0,
  JARULE_CMD_GET_FLAGS = 0xf2,
} CommandClass;

/**
 * @brief A command completion callback.
 * @tparam The result of the command operation
 * @tparam The return code from the Ja Rule device.
 * @tparam The status flags.
 * @tparam The response payload.
 *
 * If the USBCommandResult is not COMMAND_COMPLETED_OK, the remaining values
 * are undefined.
 */
typedef ola::BaseCallback4<void, USBCommandResult, uint8_t, uint8_t,
                           const ola::io::ByteString&> CommandCompleteCallback;

}  // namespace usb
}  // namespace ola

#endif  // LIBS_USB_JARULECONSTANTS_H_
