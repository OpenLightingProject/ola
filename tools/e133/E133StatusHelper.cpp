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
 * E133StatusHelper.cpp
 * Functions for dealing with E1.33 Status Codes.
 * Copyright (C) 2013 Simon Newton
 */

#include <stdint.h>
#include <string>
#include "ola/e133/E133StatusHelper.h"

namespace ola {
namespace e133 {

using std::string;

/**
 * Verify that the int is a valid E1.33 Status Code.
 */
bool IntToStatusCode(uint16_t input, E133StatusCode *status_code) {
  switch (input) {
    case ola::e133::SC_E133_ACK:
      *status_code = ola::e133::SC_E133_ACK;
      return true;
    case ola::e133::SC_E133_RDM_TIMEOUT:
      *status_code = ola::e133::SC_E133_RDM_TIMEOUT;
      return true;
    case ola::e133::SC_E133_RDM_INVALID_RESPONSE:
      *status_code = ola::e133::SC_E133_RDM_INVALID_RESPONSE;
      return true;
    case ola::e133::SC_E133_BUFFER_FULL:
      *status_code = ola::e133::SC_E133_BUFFER_FULL;
      return true;
    case ola::e133::SC_E133_UNKNOWN_UID:
      *status_code = ola::e133::SC_E133_UNKNOWN_UID;
      return true;
    case ola::e133::SC_E133_NONEXISTENT_ENDPOINT:
      *status_code = ola::e133::SC_E133_NONEXISTENT_ENDPOINT;
      return true;
    case ola::e133::SC_E133_WRONG_ENDPOINT:
      *status_code = ola::e133::SC_E133_WRONG_ENDPOINT;
      return true;
    case ola::e133::SC_E133_ACK_OVERFLOW_CACHE_EXPIRED:
      *status_code = ola::e133::SC_E133_ACK_OVERFLOW_CACHE_EXPIRED;
      return true;
    case ola::e133::SC_E133_ACK_OVERFLOW_IN_PROGRESS:
      *status_code = ola::e133::SC_E133_ACK_OVERFLOW_IN_PROGRESS;
      return true;
    case ola::e133::SC_E133_BROADCAST_COMPLETE:
      *status_code = ola::e133::SC_E133_BROADCAST_COMPLETE;
      return true;
    default:
      return false;
  }
}


/**
 * Return a text string describing this status code.
 */
string StatusCodeToString(E133StatusCode status_code) {
  switch (status_code) {
    case ola::e133::SC_E133_ACK:
     return "Acknowledged";
    case ola::e133::SC_E133_RDM_TIMEOUT:
     return "Response Timeout";
    case ola::e133::SC_E133_RDM_INVALID_RESPONSE:
     return "Invalid Response";
    case ola::e133::SC_E133_BUFFER_FULL:
     return "Buffer Full";
    case ola::e133::SC_E133_UNKNOWN_UID:
     return "Unknown UID";
    case ola::e133::SC_E133_NONEXISTENT_ENDPOINT:
     return "Endpoint doesn't exist";
    case ola::e133::SC_E133_WRONG_ENDPOINT:
     return "Wrong endpoint";
    case ola::e133::SC_E133_ACK_OVERFLOW_CACHE_EXPIRED:
     return "Ack overflow cache expired";
    case ola::e133::SC_E133_ACK_OVERFLOW_IN_PROGRESS:
     return "Ack overflow in progress";
    case ola::e133::SC_E133_BROADCAST_COMPLETE:
     return "Request was broadcast";
  }
  return "Unknown E1.33 Status Code";
}
}  // namespace e133
}  // namespace ola
