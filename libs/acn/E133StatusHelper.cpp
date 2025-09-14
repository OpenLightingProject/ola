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
#include "ola/Logging.h"
#include "ola/e133/E133StatusHelper.h"

namespace ola {
namespace e133 {

using std::string;
using ola::acn::RPTStatusVector;
using ola::e133::E133StatusCode;
using ola::e133::E133ConnectStatusCode;
using ola::rdm::RDMStatusCode;

/**
 * Verify that the int is a valid E1.33 Status Code.
 */
bool IntToStatusCode(uint16_t input, E133StatusCode *status_code) {
  if (!status_code) {
    OLA_WARN << "ola:e133::IntToStatusCode: missing status_code";
    return false;
  }

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


bool IntToConnectStatusCode(uint16_t input,
                            E133ConnectStatusCode *connect_status_code) {
  if (!connect_status_code) {
    OLA_WARN << "ola:e133::IntToConnectStatusCode: missing "
             << "connect_status_code";
    return false;
  }

  switch (input) {
    case ola::e133::CONNECT_OK:
      *connect_status_code = ola::e133::CONNECT_OK;
      return true;
    case ola::e133::CONNECT_SCOPE_MISMATCH:
      *connect_status_code = ola::e133::CONNECT_SCOPE_MISMATCH;
      return true;
    case ola::e133::CONNECT_CAPACITY_EXCEEDED:
      *connect_status_code = ola::e133::CONNECT_CAPACITY_EXCEEDED;
      return true;
    case ola::e133::CONNECT_DUPLICATE_UID:
      *connect_status_code = ola::e133::CONNECT_DUPLICATE_UID;
      return true;
    case ola::e133::CONNECT_INVALID_CLIENT_ENTRY:
      *connect_status_code = ola::e133::CONNECT_INVALID_CLIENT_ENTRY;
      return true;
    case ola::e133::CONNECT_INVALID_UID:
      *connect_status_code = ola::e133::CONNECT_INVALID_UID;
      return true;
    default:
      return false;
  }
}


string ConnectStatusCodeToString(E133ConnectStatusCode connect_status_code) {
  switch (connect_status_code) {
    case ola::e133::CONNECT_OK:
     return "Ok";
    case ola::e133::CONNECT_SCOPE_MISMATCH:
     return "Scope mismatch";
    case ola::e133::CONNECT_CAPACITY_EXCEEDED:
     return "Capacity exceeded";
    case ola::e133::CONNECT_DUPLICATE_UID:
     return "Duplicate UID";
    case ola::e133::CONNECT_INVALID_CLIENT_ENTRY:
     return "Invalid client entry";
    case ola::e133::CONNECT_INVALID_UID:
     return "Invalid UID";
  }
  return "Unknown E1.33 Connect Status Code";
}


bool IntToRPTStatusCode(uint16_t input,
                        RPTStatusVector *rpt_status_code) {
  if (!rpt_status_code) {
    OLA_WARN << "ola:e133::IntToRPTStatusCode: missing rpt_status_code";
    return false;
  }

  switch (input) {
    case ola::acn::VECTOR_RPT_STATUS_UNKNOWN_RPT_UID:
      *rpt_status_code = ola::acn::VECTOR_RPT_STATUS_UNKNOWN_RPT_UID;
      return true;
    case ola::acn::VECTOR_RPT_STATUS_RDM_TIMEOUT:
      *rpt_status_code = ola::acn::VECTOR_RPT_STATUS_RDM_TIMEOUT;
      return true;
    case ola::acn::VECTOR_RPT_STATUS_RDM_INVALID_RESPONSE:
      *rpt_status_code = ola::acn::VECTOR_RPT_STATUS_RDM_INVALID_RESPONSE;
      return true;
    case ola::acn::VECTOR_RPT_STATUS_UNKNOWN_RDM_UID:
      *rpt_status_code = ola::acn::VECTOR_RPT_STATUS_UNKNOWN_RDM_UID;
      return true;
    case ola::acn::VECTOR_RPT_STATUS_UNKNOWN_ENDPOINT:
      *rpt_status_code = ola::acn::VECTOR_RPT_STATUS_UNKNOWN_ENDPOINT;
      return true;
    case ola::acn::VECTOR_RPT_STATUS_BROADCAST_COMPLETE:
      *rpt_status_code = ola::acn::VECTOR_RPT_STATUS_BROADCAST_COMPLETE;
      return true;
    case ola::acn::VECTOR_RPT_STATUS_UNKNOWN_VECTOR:
      *rpt_status_code = ola::acn::VECTOR_RPT_STATUS_UNKNOWN_VECTOR;
      return true;
    case ola::acn::VECTOR_RPT_STATUS_INVALID_MESSAGE:
      *rpt_status_code = ola::acn::VECTOR_RPT_STATUS_INVALID_MESSAGE;
      return true;
    case ola::acn::VECTOR_RPT_STATUS_INVALID_COMMAND_CLASS:
      *rpt_status_code = ola::acn::VECTOR_RPT_STATUS_INVALID_COMMAND_CLASS;
      return true;
    default:
      return false;
  }
}


string RPTStatusCodeToString(RPTStatusVector rpt_status_code) {
  switch (rpt_status_code) {
    case ola::acn::VECTOR_RPT_STATUS_UNKNOWN_RPT_UID:
     return "Unknown RPT UID";
    case ola::acn::VECTOR_RPT_STATUS_RDM_TIMEOUT:
     return "RDM Timeout";
    case ola::acn::VECTOR_RPT_STATUS_RDM_INVALID_RESPONSE:
     return "RDM Invalid Response";
    case ola::acn::VECTOR_RPT_STATUS_UNKNOWN_RDM_UID:
     return "Unknown RDM UID";
    case ola::acn::VECTOR_RPT_STATUS_UNKNOWN_ENDPOINT:
     return "Unknown Endpoint";
    case ola::acn::VECTOR_RPT_STATUS_BROADCAST_COMPLETE:
     return "Broadcast Complete";
    case ola::acn::VECTOR_RPT_STATUS_UNKNOWN_VECTOR:
     return "Unknown Vector";
    case ola::acn::VECTOR_RPT_STATUS_INVALID_MESSAGE:
     return "Invalid Message";
    case ola::acn::VECTOR_RPT_STATUS_INVALID_COMMAND_CLASS:
     return "Invalid Command Class";
  }
  return "Unknown E1.33 RPT Status Code";
}


bool RPTStatusCodeToRDMStatusCode(RPTStatusVector rpt_status_code,
                                  RDMStatusCode *rdm_status_code) {
  if (!rdm_status_code) {
    OLA_WARN << "ola:e133::RPTStatusCodeToRDMStatusCode: missing "
             << "rdm_status_code";
    return false;
  }

  // TODO(Peter): Fill in the gaps, possibly adding additional RDMStatusCodes
  // if required
  switch (rpt_status_code) {
//    case ola::acn::VECTOR_RPT_STATUS_UNKNOWN_RPT_UID:
//      *rdm_status_code = ola::rdm::;
//      return true;
    case ola::acn::VECTOR_RPT_STATUS_RDM_TIMEOUT:
      *rdm_status_code = ola::rdm::RDM_TIMEOUT;
      return true;
    case ola::acn::VECTOR_RPT_STATUS_RDM_INVALID_RESPONSE:
      *rdm_status_code = ola::rdm::RDM_INVALID_RESPONSE;
      return true;
    case ola::acn::VECTOR_RPT_STATUS_UNKNOWN_RDM_UID:
      *rdm_status_code = ola::rdm::RDM_UNKNOWN_UID;
      return true;
//    case ola::acn::VECTOR_RPT_STATUS_UNKNOWN_ENDPOINT:
//      *rdm_status_code = ola::rdm::;
//      return true;
    case ola::acn::VECTOR_RPT_STATUS_BROADCAST_COMPLETE:
      *rdm_status_code = ola::rdm::RDM_WAS_BROADCAST;
      return true;
//    case ola::acn::VECTOR_RPT_STATUS_UNKNOWN_VECTOR:
//      *rdm_status_code = ola::rdm::;
//      return true;
//    case ola::acn::VECTOR_RPT_STATUS_INVALID_MESSAGE:
//      *rdm_status_code = ola::rdm::;
//      return true;
    case ola::acn::VECTOR_RPT_STATUS_INVALID_COMMAND_CLASS:
      *rdm_status_code = ola::rdm::RDM_INVALID_COMMAND_CLASS;
      return true;
    default:
      return false;
  }
}
}  // namespace e133
}  // namespace ola
