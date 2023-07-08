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
 * E133Enums.h
 * Enums for use with E1.33
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_E133_E133ENUMS_H_
#define INCLUDE_OLA_E133_E133ENUMS_H_

namespace ola {
namespace e133 {

// Table A-6, Discovery Stats
enum DiscoveryState {
  DISCOVERY_INCOMPLETE = 0,
  DISCOVERY_INCREMENTAL = 2,
  DISCOVERY_FULL = 3,
  DISCOVERY_NOT_ACTIVE = 4,
};

// Table A-7, Endpoint Modes.
enum EndpointMode {
  ENDPOINT_MODE_DISABLED = 0,
  ENDPOINT_MODE_INPUT = 1,
  ENDPOINT_MODE_OUTPUT = 2,
};

// TODO(Peter): Check that this no longer exists
// Table A-9 E1.33 Status Codes
enum E133StatusCode {
  SC_E133_ACK = 0x0000,
  SC_E133_RDM_TIMEOUT = 0x0001,
  SC_E133_RDM_INVALID_RESPONSE = 0x0002,
  SC_E133_BUFFER_FULL = 0x0003,
  SC_E133_UNKNOWN_UID = 0x0004,
  SC_E133_NONEXISTENT_ENDPOINT = 0x0005,
  SC_E133_WRONG_ENDPOINT = 0x0006,
  SC_E133_ACK_OVERFLOW_CACHE_EXPIRED = 0x0007,
  SC_E133_ACK_OVERFLOW_IN_PROGRESS = 0x0008,
  SC_E133_BROADCAST_COMPLETE = 0x0009,
};

// Table A-19 E1.33 Connection Status Codes for Broker Connect
enum E133ConnectStatusCode {
  CONNECT_OK = 0x0000,
  CONNECT_SCOPE_MISMATCH = 0x0001,
  CONNECT_CAPACITY_EXCEEDED = 0x0002,
  CONNECT_DUPLICATE_UID = 0x0003,
  CONNECT_INVALID_CLIENT_ENTRY = 0x0004,
  CONNECT_INVALID_UID = 0x0005,
};

// Table A-20 E1.33 Status Codes for Dynamic UID Mapping
enum E133DynamicUIDStatusCode {
  DYNAMIC_UID_STATUS_OK = 0x0000,
  DYNAMIC_UID_STATUS_INVALID_REQUEST = 0x0001,
  DYNAMIC_UID_STATUS_UID_NOT_FOUND = 0x0002,
  DYNAMIC_UID_STATUS_DUPLICATE_RID = 0x0003,
  DYNAMIC_UID_STATUS_CAPACITY_EXHAUSTED = 0x0004,
};

// Table A-22 E1.33 RPT Client Type Codes
enum E133RPTClientTypeCode {
  RPT_CLIENT_TYPE_DEVICE = 0x00,
  RPT_CLIENT_TYPE_CONTROLLER = 0x01,
};

// Table A-24 E1.33 Client Disconnect Reason Codes
enum E133DisconnectStatusCode {
  DISCONNECT_SHUTDOWN = 0x0000,
  DISCONNECT_CAPACITY_EXHAUSTED = 0x0001,
  DISCONNECT_HARDWARE_FAULT = 0x0002,
  DISCONNECT_SOFTWARE_FAULT = 0x0003,
  DISCONNECT_SOFTWARE_RESET = 0x0004,
  DISCONNECT_INCORRECT_SCOPE = 0x0005,
  DISCONNECT_RPT_RECONFIGURE = 0x0006,
  DISCONNECT_LLRP_RECONFIGURE = 0x0007,
  DISCONNECT_USER_RECONFIGURE = 0x0008,
};

// The max size of an E1.33 Status string.
enum {
  MAX_E133_STATUS_STRING_SIZE = 64
};

// The E1.33 version..
enum {
  E133_VERSION = 1
};
}  // namespace e133
}  // namespace ola
#endif  // INCLUDE_OLA_E133_E133ENUMS_H_
