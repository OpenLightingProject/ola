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

#ifndef PLUGINS_USBDMX_JARULECONSTANTS_H_
#define PLUGINS_USBDMX_JARULECONSTANTS_H_

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
namespace jarule {

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
} CommandResult;

/**
 * @brief The Ja Rule command set.
 */
typedef enum {
  RESET_DEVICE = 0x00,
  SET_MODE = 0x01,
  GET_UID = 0x02,
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

}  // namespace jarule
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola

#endif  // PLUGINS_USBDMX_JARULECONSTANTS_H_
