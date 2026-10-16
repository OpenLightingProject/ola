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
 * SigNetEnums.h
 * General enums used in Sig-Net packets
 * Copyright (C) 2026 Peter Newman
 */

#ifndef PLUGINS_SIGNET_SIGNETENUMS_H_
#define PLUGINS_SIGNET_SIGNETENUMS_H_
/**
 * @addtogroup signet
 * @{
 * @file SigNetEnumss.h
 * @brief Sig-Net enum values.
 * @}
 */

#include <stdint.h>

namespace ola {
namespace plugin {
namespace signet {

/**
 * @addtogroup signet
 * @{
 */

static const uint8_t MAX_SIGNET_STRING_LENGTH = 64;

static const uint16_t SIGNET_ROOT_ENDPOINT = 0x0000;
static const uint16_t SIGNET_BROADCAST_ENDPOINT = 0xFFFF;

static const uint16_t SIGNET_MIN_UNIVERSE = 1;
static const uint16_t SIGNET_MAX_UNIVERSE = 63999;

static const uint32_t SIGNET_ROOT_STATUS_HARDWARE_FAULT = 0x00000001;
static const uint32_t SIGNET_ROOT_STATUS_FACTORY_DEFAULT = 0x00000002;
static const uint32_t SIGNET_ROOT_STATUS_CONFIG_LOCKED = 0x00000004;
static const uint32_t SIGNET_ROOT_STATUS_OPEN_MODE = 0x00000008;

static const uint32_t SIGNET_ROOT_ROLE_CAPABILITY_NODE = 0x00000001;
static const uint32_t SIGNET_ROOT_ROLE_CAPABILITY_SENDER = 0x00000002;
static const uint32_t SIGNET_ROOT_ROLE_CAPABILITY_MANAGER = 0x00000004;
static const uint32_t SIGNET_ROOT_ROLE_CAPABILITY_VISUALISER = 0x00000008;
static const uint32_t SIGNET_ROOT_ROLE_CAPABILITY_RESERVED_1 = 0x00000010;
static const uint32_t SIGNET_ROOT_ROLE_CAPABILITY_RESERVED_2 = 0x00000020;
static const uint32_t SIGNET_ROOT_ROLE_CAPABILITY_ROOT_FIRMWARE_SUPPORT = 0x00000040;
static const uint32_t SIGNET_ROOT_ROLE_CAPABILITY_OPEN_MODE_SUPPORT = 0x00000080;

static const uint32_t SIGNET_ENDPOINT_CAPABILITY_CONSUME_LEVEL = 0x00000001;
static const uint32_t SIGNET_ENDPOINT_CAPABILITY_SUPPLY_LEVEL = 0x00000002;
static const uint32_t SIGNET_ENDPOINT_CAPABILITY_CONSUME_RDM = 0x00000004;
static const uint32_t SIGNET_ENDPOINT_CAPABILITY_SUPPLY_RDM = 0x00000008;
static const uint32_t SIGNET_ENDPOINT_CAPABILITY_VIRTUAL = 0x00000010;

static const uint8_t SIGNET_ENDPOINT_DIRECTION_DIRECTION_BITMASK = 0x03;
static const uint8_t SIGNET_ENDPOINT_DIRECTION_DISABLED = 0x00;
static const uint8_t SIGNET_ENDPOINT_DIRECTION_CONSUMER = 0x01;
static const uint8_t SIGNET_ENDPOINT_DIRECTION_SUPPLIER = 0x02;
static const uint8_t SIGNET_ENDPOINT_DIRECTION_FALLBACK = 0x03;
static const uint8_t SIGNET_ENDPOINT_DIRECTION_RDM_ENABLE = 0x04;

static const uint32_t SIGNET_ENDPOINT_STATUS_TX_RX = 0x00000001;
static const uint32_t SIGNET_ENDPOINT_STATUS_HARDWARE_FAULT = 0x00000002;
static const uint32_t SIGNET_ENDPOINT_STATUS_CONFIG_LOCKED = 0x00000004;
static const uint32_t SIGNET_ENDPOINT_STATUS_RX_TID_LEVEL = 0x00000008;
static const uint32_t SIGNET_ENDPOINT_STATUS_RX_MULTI_TID_LEVEL = 0x00000010;
static const uint32_t SIGNET_ENDPOINT_STATUS_FALLBACK = 0x00000020;
static const uint32_t SIGNET_ENDPOINT_STATUS_FAILOVER = 0x00000040;

/**
 * @brief A set of values representing Sig-Net COAP Options.
 * @note See section 8.3 of Sig-Net for more information.
 */
typedef enum {
  SIGNET_SECURITY_MODE = 2076, /**< Sig-Net-Security-Mode */
  SIGNET_SENDER_ID = 2108, /**< Sig-Net-Sender-ID */
  SIGNET_MFG_CODE = 2140, /**< Sig-Net-Mfg-Code */
  SIGNET_SESSION_ID = 2172, /**< Sig-Net-Session-ID */
  SIGNET_SEQ_NUM = 2204, /**< Sig-Net-Seq-Num */
  SIGNET_AUTH = 2236, /**< Sig-Net-Auth */
} signet_coap_options;

/**
 * @brief A set of values representing Sig-Net-Security-Mode choices.
 * @note See section 8.3 of Sig-Net for more information.
 */
typedef enum {
  SIGNET_SECURITY_MODE_SHA256 = 0x00, /**< Plaintext with, HMAC-SHA256 */
  SIGNET_SECURITY_MODE_OPEN = 0x01, /**< Open, unauthenticated packets */
  SIGNET_SECURITY_MODE_OFFBOARDED = 0xFF, /**< Offboarded device */
} signet_security_mode;


/**
 * @brief A set of values representing Sig-Net query levels.
 * @note See section 10.2.3 of Sig-Net for more information.
 */
typedef enum {
  SIGNET_QUERY_HEARTBEAT = 0x00, /**< Heartbeat */
  SIGNET_QUERY_CONFIG = 0x01, /**< Config */
  SIGNET_QUERY_FULL = 0x02, /**< Full */
  SIGNET_QUERY_EXTENDED = 0x03, /**< Extended */
} signet_query_mode;


/**
 * @brief A set of values representing Sig-Net TOD control commands.
 * @note See section 11.3.3 of Sig-Net for more information.
 */
typedef enum {
  SIGNET_TOD_CONTROL_SEND = 0x00, /**< Send */
  SIGNET_TOD_CONTROL_FORCE = 0x01, /**< Force */
} signet_tod_control;


typedef enum {
  SIGNET_TEXT_ENCODING_UTF8_ASCII = 0x00, /**< UTF-8/ASCII */
} signet_text_encoding;

/**
 * @}
 */
}  // namespace signet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SIGNET_SIGNETENUMS_H_
