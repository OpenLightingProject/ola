/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * E133StatusHelper.cpp
 * Functions for dealing with E1.33 Status Codes.
 * Copyright (C) 2013 Simon Newton
 */

#include <stdint.h>
#include <string>
#include "plugins/e131/e131/E133StatusHelper.h"

namespace ola {
namespace plugin {
namespace e131 {

using std::string;

/**
 * Verify that the int is a valid E1.33 Status Code.
 */
bool IntToStatusCode(uint16_t input, E133StatusCode *status_code) {
  switch (input) {
    case SC_E133_ACK:
      *status_code = SC_E133_ACK;
      return true;
    case SC_E133_RDM_TIMEOUT:
      *status_code = SC_E133_RDM_TIMEOUT;
      return true;
    case SC_E133_RDM_INVALID_RESPONSE:
      *status_code = SC_E133_RDM_INVALID_RESPONSE;
      return true;
    case SC_E133_BUFFER_FULL:
      *status_code = SC_E133_BUFFER_FULL;
      return true;
    case SC_E133_UNKNOWN_UID:
      *status_code = SC_E133_UNKNOWN_UID;
      return true;
    case SC_E133_NONEXISTANT_ENDPOINT:
      *status_code = SC_E133_NONEXISTANT_ENDPOINT;
      return true;
    case SC_E133_WRONG_ENDPOINT:
      *status_code = SC_E133_WRONG_ENDPOINT;
      return true;
    case SC_E133_ACK_OVERFLOW_CACHE_EXPIRED:
      *status_code = SC_E133_ACK_OVERFLOW_CACHE_EXPIRED;
      return true;
    case SC_E133_ACK_OVERFLOW_IN_PROGRESS:
      *status_code = SC_E133_ACK_OVERFLOW_IN_PROGRESS;
      return true;
    case SC_E133_BROADCAST_COMPLETE:
      *status_code = SC_E133_BROADCAST_COMPLETE;
      return true;
    default:
      return false;
  }
}


/**
 * Return a text string describing this status code.
 */
string StatusMessageIdToString(E133StatusCode status_code) {
  switch (status_code) {
    case SC_E133_ACK:
     return "Acknowledged";
    case SC_E133_RDM_TIMEOUT:
     return "Response Timeout";
    case SC_E133_RDM_INVALID_RESPONSE:
     return "Invalid Response";
    case SC_E133_BUFFER_FULL:
     return "Buffer Full";
    case SC_E133_UNKNOWN_UID:
     return "Unknown UID";
    case SC_E133_NONEXISTANT_ENDPOINT:
     return "Endpoint doesn't exist";
    case SC_E133_WRONG_ENDPOINT:
     return "Wrong endpoint";
    case SC_E133_ACK_OVERFLOW_CACHE_EXPIRED:
     return "Ack overflow cache expired";
    case SC_E133_ACK_OVERFLOW_IN_PROGRESS:
     return "Ack overflow in progress";
    case SC_E133_BROADCAST_COMPLETE:
     return "Request was broadcast";
  }
  return "Unknown E1.33 Status Code";
}
}  // e131
}  // plugin
}  // ola
