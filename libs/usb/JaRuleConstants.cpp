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
 * JaRuleConstants.cpp
 * Constants used with Ja Rule devices.
 * Copyright (C) 2015 Simon Newton
 */

#include "libs/usb/JaRuleConstants.h"
#include <ostream>

namespace ola {
namespace usb {

using std::ostream;

ostream& operator<<(ostream& os, const USBCommandResult &result) {
  switch (result) {
    case COMMAND_RESULT_OK:
      os << "OK";
      break;
    case COMMAND_RESULT_MALFORMED:
      os << "MALFORMED";
      break;
    case COMMAND_RESULT_SEND_ERROR:
      os << "SEND_ERROR";
      break;
    case COMMAND_RESULT_QUEUE_FULL:
      os << "QUEUE_FULL";
      break;
    case COMMAND_RESULT_TIMEOUT:
      os << "TIMEOUT";
      break;
    case COMMAND_RESULT_CLASS_MISMATCH:
      os << "CLASS_MISMATCH";
      break;
    case COMMAND_RESULT_CANCELLED:
      os << "CANCELLED";
      break;
    case COMMAND_RESULT_INVALID_PORT:
      os << "INVALID_PORT";
      break;
    default:
      os << "Unknown";
  }
  os << " (" << static_cast<int>(result) << ")";
  return os;
}

ostream& operator<<(ostream& os, const CommandClass &command_class) {
  switch (command_class) {
    case JARULE_CMD_RESET_DEVICE:
      os << "RESET_DEVICE";
      break;
    case JARULE_CMD_SET_MODE:
      os << "SET_MODE";
      break;
    case JARULE_CMD_GET_HARDWARE_INFO:
      os << "GET_HARDWARE_INFO";
      break;
    case JARULE_CMD_RUN_SELF_TEST:
      os << "RUN_SELF_TEST";
      break;
    case JARULE_CMD_SET_BREAK_TIME:
      os << "SET_BREAK_TIME";
      break;
    case JARULE_CMD_GET_BREAK_TIME:
      os << "GET_BREAK_TIME";
      break;
    case JARULE_CMD_SET_MARK_TIME:
      os << "SET_MARK_TIME";
      break;
    case JARULE_CMD_GET_MARK_TIME:
      os << "GET_MARK_TIME";
      break;
    case JARULE_CMD_SET_RDM_BROADCAST_TIMEOUT:
      os << "SET_RDM_BROADCAST_TIMEOUT";
      break;
    case JARULE_CMD_GET_RDM_BROADCAST_TIMEOUT:
      os << "GET_RDM_BROADCAST_TIMEOUT";
      break;
    case JARULE_CMD_SET_RDM_RESPONSE_TIMEOUT:
      os << "SET_RDM_RESPONSE_TIMEOUT";
      break;
    case JARULE_CMD_GET_RDM_RESPONSE_TIMEOUT:
      os << "GET_RDM_RESPONSE_TIMEOUT";
      break;
    case JARULE_CMD_SET_RDM_DUB_RESPONSE_LIMIT:
      os << "SET_RDM_DUB_RESPONSE_LIMIT";
      break;
    case JARULE_CMD_GET_RDM_DUB_RESPONSE_LIMIT:
      os << "GET_RDM_DUB_RESPONSE_LIMIT";
      break;
    case JARULE_CMD_SET_RDM_RESPONDER_DELAY:
      os << "SET_RDM_RESPONDER_DELAY";
      break;
    case JARULE_CMD_GET_RDM_RESPONDER_DELAY:
      os << "GET_RDM_RESPONDER_DELAY";
      break;
    case JARULE_CMD_SET_RDM_RESPONDER_JITTER:
      os << "SET_RDM_RESPONDER_JITTER";
      break;
    case JARULE_CMD_GET_RDM_RESPONDER_JITTER:
      os << "GET_RDM_RESPONDER_JITTER";
      break;
    case JARULE_CMD_TX_DMX:
      os << "TX_DMX";
      break;
    case JARULE_CMD_RDM_DUB_REQUEST:
      os << "RDM_DUB_REQUEST";
      break;
    case JARULE_CMD_RDM_REQUEST:
      os << "RDM_REQUEST";
      break;
    case JARULE_CMD_RDM_BROADCAST_REQUEST:
      os << "RDM_BROADCAST_REQUEST";
      break;
    case JARULE_CMD_ECHO:
      os << "ECHO";
      break;
    case JARULE_CMD_GET_FLAGS:
      os << "GET_FLAGS";
      break;
    default:
      os << "Unknown";
  }
  os << " (" << static_cast<int>(command_class) << ")";
  return os;
}

ostream& operator<<(ostream& os, const JaRuleReturnCode &rc) {
  switch (rc) {
    case RC_OK:
      os << "OK";
      break;
    case RC_UNKNOWN:
      os << "UNKNOWN";
      break;
    case RC_BUFFER_FULL:
      os << "BUFFER_FULL";
      break;
    case RC_BAD_PARAM:
      os << "BAD_PARAM";
      break;
    case RC_TX_ERROR:
      os << "TX_ERROR";
      break;
    case RC_RDM_TIMEOUT:
      os << "RDM_TIMEOUT";
      break;
    case RC_RDM_BCAST_RESPONSE:
      os << "RDM_BCAST_RESPONSE";
      break;
    case RC_RDM_INVALID_RESPONSE:
      os << "RDM_INVALID_RESPONSE";
      break;
    case RC_INVALID_MODE:
      os << "INVALID_MODE";
      break;
    default:
      os << "Unknown";
  }
  os << " (" << static_cast<int>(rc) << ")";
  return os;
}
}  // namespace usb
}  // namespace ola
