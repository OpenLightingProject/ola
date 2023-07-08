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
 * ACNVectors.h
 * Vectors used in ACN PDUs
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_ACN_ACNVECTORS_H_
#define INCLUDE_OLA_ACN_ACNVECTORS_H_

/**
 * @addtogroup acn
 * @{
 * @file ACNVectors.h
 * @brief ACN Vector values.
 * @}
 */

#include <stdint.h>

namespace ola {
namespace acn {

/**
 * @addtogroup acn
 * @{
 */

/**
 * @brief ACN vectors used at the root layer.
 */
enum RootVector {
  VECTOR_ROOT_E131_REV2 = 3,  /**< Draft E1.31, used by some old gear. */
  VECTOR_ROOT_E131 = 4,  /**< E1.31 (sACN) */
  VECTOR_ROOT_RPT = 5,  /**< E1.33 (RPT) */
  VECTOR_ROOT_NULL = 6,  /**< NULL (empty) root */
  VECTOR_ROOT_BROKER = 9,  /**< E1.33 (Broker) */
  VECTOR_ROOT_LLRP = 0x0A,  /**< E1.33 (LLRP) */
  VECTOR_ROOT_EPT = 0x0B,  /**< E1.33 (EPT) */
};

/**
 * @brief Vectors used at the DMP layer.
 */
enum DMPVector {
  DMP_GET_PROPERTY_VECTOR = 1,  /**< DMP Get */
  DMP_SET_PROPERTY_VECTOR = 2,  /**< DMP Set */
};

/**
 * @brief Vectors used at the E1.31 layer.
 */
enum E131Vector {
  VECTOR_E131_DATA = 2,  /**< DMP data (DATA_PACKET_VECTOR) */
  VECTOR_E131_SYNC = 3,  /**< Sync data (SYNC_PACKET_VECTOR) */
  VECTOR_E131_DISCOVERY = 4,  /**< Discovery data (DISCOVERY_PACKET_VECTOR) */
};

/**
 * @brief Vectors used at the E1.33 layer.
 */
enum E133Vector {
  VECTOR_FRAMING_RDMNET = 1,  /**< RDMNet data */
  VECTOR_FRAMING_STATUS = 2,  /**< Status message */
  VECTOR_FRAMING_CONTROLLER = 3,  /**< Controller message */
  VECTOR_FRAMING_CHANGE_NOTIFICATION = 4,  /**< Controller change message */
};

/**
 * @brief Vectors used at the E1.33 Controller layer.
 */
enum E133ControllerVector {
  VECTOR_CONTROLLER_FETCH_DEVICES = 1,  /**< Fetch devices message */
  VECTOR_CONTROLLER_DEVICE_LIST = 2,  /**< Device list message */
  VECTOR_CONTROLLER_DEVICE_ACQUIRED = 3,  /**< Device acquired message */
  VECTOR_CONTROLLER_DEVICE_RELEASED = 4,  /**< Device released message */
  VECTOR_CONTROLLER_EXPECT_MASTER = 5,  /**< Expect master message */
};

/**
 * @brief Vectors used at the E1.33 LLRP layer.
 */
enum LLRPVector {
  VECTOR_LLRP_PROBE_REQUEST = 1,  /**< LLRP Probe Request */
  VECTOR_LLRP_PROBE_REPLY = 2,  /**< LLRP Probe Reply */
  VECTOR_LLRP_RDM_CMD = 3,  /**< LLRP RDM Command */
};

/**
 * @brief Vectors used at the E1.33 Broker layer.
 */
enum BrokerVector {
  VECTOR_BROKER_CONNECT = 0x0001,  /**< Broker Client Connect */
  VECTOR_BROKER_CONNECT_REPLY = 0x0002,  /**< Broker Connect Reply */
  VECTOR_BROKER_CLIENT_ENTRY_UPDATE = 0x0003,  /**< Broker Client Entry Update */
  VECTOR_BROKER_REDIRECT_V4 = 0x0004,  /**< Broker Client Redirect IPv4 */
  VECTOR_BROKER_REDIRECT_V6 = 0x0005,  /**< Broker Client Redirect IPv6 */
  VECTOR_BROKER_FETCH_CLIENT_LIST = 0x0006,  /**< Broker Fetch Client List */
  VECTOR_BROKER_CONNECTED_CLIENT_LIST = 0x0007,  /**< Broker Connected Client List */
  VECTOR_BROKER_CLIENT_ADD = 0x0008,  /**< Broker Client Incremental Addition */
  VECTOR_BROKER_CLIENT_REMOVE = 0x0009,  /**< Broker Client Incremental Deletion */
  VECTOR_BROKER_CLIENT_ENTRY_CHANGE = 0x000A,  /**< Broker Client Entry Change */
  VECTOR_BROKER_REQUEST_DYNAMIC_UIDS = 0x000B,  /**< Broker Request Dynamic UID Assignment */
  VECTOR_BROKER_ASSIGNED_DYNAMIC_UIDS = 0x000C,  /**< Broker Dynamic UID Assignment List */
  VECTOR_BROKER_FETCH_DYNAMIC_UID_LIST = 0x000D,  /**< Broker Fetch Dynamic UID Assignment List */
  VECTOR_BROKER_DISCONNECT = 0x000E,  /**< Broker Client Disconnect */
  VECTOR_BROKER_NULL = 0x000F,  /**< Broker Client Null */
};

// Table A-8, RPT PDU Vector
/**
 * @brief Vectors used at the E1.33 RPT layer.
 */
enum RPTVector {
  VECTOR_RPT_REQUEST = 1,  /**< RPT Request */
  VECTOR_RPT_STATUS = 2,  /**< RPT Status */
  VECTOR_RPT_NOTIFICATION = 3,  /**< RPT Notification */
};

// Table A-9, RPT Request PDU Vector
/**
 * @brief Vectors used at the E1.33 RPT Request layer.
 */
enum RPTRequestVector {
  VECTOR_REQUEST_RDM_CMD = 1,  /**< RPT Request RDM Command */
};

// Table A-10, RPT Status PDU Vector
/**
 * @brief Vectors used at the E1.33 RPT Status layer.
 */
enum RPTStatusVector {
  VECTOR_RPT_STATUS_UNKNOWN_RPT_UID = 0x0001,  /**< RPT Status Unknown RPT UID */
  VECTOR_RPT_STATUS_RDM_TIMEOUT = 0x0002,  /**< RPT Status RDM Timeout */
  VECTOR_RPT_STATUS_RDM_INVALID_RESPONSE = 0x0003,  /**< RPT Status RDM Invalid Response */
  VECTOR_RPT_STATUS_UNKNOWN_RDM_UID = 0x0004,  /**< RPT Status Unknown RDM UID */
  VECTOR_RPT_STATUS_UNKNOWN_ENDPOINT = 0x0005,  /**< RPT Status Unknown Endpoint */
  VECTOR_RPT_STATUS_BROADCAST_COMPLETE = 0x0006,  /**< RPT Status Broadcast Complete */
  VECTOR_RPT_STATUS_UNKNOWN_VECTOR = 0x0007,  /**< RPT Status Unknown Vector */
  VECTOR_RPT_STATUS_INVALID_MESSAGE = 0x0008,  /**< RPT Status Invalid Message */
  VECTOR_RPT_STATUS_INVALID_COMMAND_CLASS = 0x0009,  /**< RPT Status Invalid Command Class */
};

// Table A-11, RPT Notification PDU Vector
/**
 * @brief Vectors used at the E1.33 RPT Notification layer.
 */
enum RPTNotificationVector {
  VECTOR_NOTIFICATION_RDM_CMD = 1,  /**< RPT Notification RDM Command */
};

/**
 * @brief Vectors used at the E1.33 RDM Command layer.
 */
enum RDMCommandVector {
  VECTOR_RDM_CMD_RDM_DATA = 0xCC,  /**< E1.33 RDM Command */
};

// Table A-21, Client Protocol Codes
// These aren't fully named as vectors in the standard, but are used as such so
// we put them in here
/**
 * @brief Vectors used at the E1.33 Broker Client Entry layer.
 */
enum ClientProtocolCode {
  CLIENT_PROTOCOL_RPT = 0x00000005,  /**< Broker RPT Client Entry */
  CLIENT_PROTOCOL_EPT = 0x0000000B,  /**< Broker EPT Client Entry */
};

/**
 * @}
 */
}  // namespace acn
}  // namespace ola

#endif  // INCLUDE_OLA_ACN_ACNVECTORS_H_
