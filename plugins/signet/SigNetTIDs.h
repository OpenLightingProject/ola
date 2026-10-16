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
 * SigNetTIDs.h
 * TIDs used in Sig-Net packets
 * Copyright (C) 2026 Peter Newman
 */

#ifndef PLUGINS_SIGNET_SIGNETTIDS_H_
#define PLUGINS_SIGNET_SIGNETTIDS_H_

/**
 * @addtogroup signet
 * @{
 * @file SigNetTIDs.h
 * @brief Sig-Net TID values.
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

// Section 17, TID Dictionary
/**
 * @brief Sig-Net TIDs used at the root level.
 */
enum RootTID {
  TID_POLL = 0x0001,  /**< TID Poll */
  TID_POLL_REPLY = 0x0002,  /**< TID Poll Reply */
  TID_SET_REPLY = 0x0003,  /**< TID Set Reply */

  TID_LEVEL = 0x0101,  /**< TID Level */
  TID_PRIORITY = 0x0102,  /**< TID Priority */

  TID_SYNC = 0x0201,  /**< TID Sync */
  TID_TIMECODE = 0x0202,  /**< TID Timecode */
  TID_UNIVERSE = 0x0203,  /**< TID Universe */
  TID_OSC = 0x0204,  /**< TID OSC */

  TID_RDM_COMMAND = 0x0301,  /**< TID RDM Command */
  TID_RDM_RESPONSE = 0x0302,  /**< TID RDM Response */
  TID_RDM_TOD_CONTROL = 0x0303,  /**< TID ToD Control */
  TID_RDM_TOD_DATA = 0x0304,  /**< TID RDM ToD Data */
  TID_RDM_EP_CONFIG = 0x0305,  /**< TID RDM Endpoint Config */
  TID_RDM_FLOW_CONTROL = 0x0306,  /**< TID RDM Flow Control */

  TID_RT_OFFBOARD = 0x0401,  /**< TID Root Offboard */

  TID_NW_MAC_ADDRESS = 0x0501,  /**< TID Network MAC Address */
  TID_NW_IPV4_MODE = 0x0502,  /**< TID Network IPv4 Mode */
  TID_NW_IPV4_ADDRESS = 0x0503,  /**< TID Network IPv4 Address */
  TID_NW_IPV4_NETMASK = 0x0504,  /**< TID Network IPv4 Netmask */
  TID_NW_IPV4_GATEWAY = 0x0505,  /**< TID Network IPv4 Gateway */
  TID_NW_IPV4_CURRENT = 0x0506,  /**< TID Network IPv4 Current Details */

  TID_NW_IPV6_MODE = 0x0581,  /**< TID Network IPv6 Mode */
  TID_NW_IPV6_ADDRESS = 0x0582,  /**< TID Network IPv6 Address */
  TID_NW_IPV6_PREFIX = 0x0583,  /**< TID Network IPv6 Prefix */
  TID_NW_IPV6_GATEWAY = 0x0584,  /**< TID Network IPv6 Gateway */
  TID_NW_IPV6_CURRENT = 0x0585,  /**< TID Network IPv6 Current Details */

  TID_RT_SUPPORTED_TIDS = 0x0601,  /**< TID Root Supported TIDs */
  TID_RT_ENDPOINT_COUNT = 0x0602,  /**< TID Root Endpoint Count */
  TID_RT_PROTOCOL_VERSION = 0x0603,  /**< TID Root Protocol Version */
  TID_RT_FIRMWARE_VERSION = 0x0604,  /**< TID Root Firmware Version */
  TID_RT_DEVICE_LABEL = 0x0605,  /**< TID Root Device Label */
  TID_RT_MULT_OVERRIDE = 0x0606,  /**< TID Root Multicast Override */
  TID_RT_IDENTIFY = 0x0607,  /**< TID Root Identify*/
  TID_RT_STATUS = 0x0608,  /**< TID Root Status */
  TID_RT_ROLE_CAPABILITY = 0x0609,  /**< TID Root Role Capability */
  TID_RT_REBOOT = 0x060A,  /**< TID Root Reboot */
  TID_RT_MODEL_NAME         = 0x060B,  /**< TID Root Model Name */
  TID_RT_SCOPE = 0x060C,  /**< TID Root URI Scope */
  TID_RT_OTW_CAPABILITY = 0x060D,  /**< TID Root Over-the-Wire Capability */

  TID_EP_UNIVERSE = 0x0901,  /**< TID Endpoint Universe */
  TID_EP_LABEL = 0x0902,  /**< TID Endpoint Label */
  TID_EP_MULT_OVERRIDE = 0x0903,  /**< TID Endpoint Multicast IPv4 Override */
  TID_EP_CAPABILITY = 0x0904,  /**< TID Endpoint Port Capability */
  TID_EP_DIRECTION = 0x0905,  /**< TID Endpoint Direction */
  TID_EP_INPUT_PRIORITY = 0x0906,  /**< TID Endpoint Input Priority */
  TID_EP_STATUS = 0x0907,  /**< TID Endpoint Status */
  TID_EP_FAILOVER = 0x0908,  /**< TID Endpoint Failover */
  TID_EP_DMX_TIMING = 0x0909,  /**< TID Endpoint DMX Timing */
  TID_EP_REFRESH_CAPABILITY = 0x090A,  /**< TID Endpoint Refresh Capability */
  TID_EP_PROTOCOL = 0x090B,  /**< TID Endpoint Protocol */
  TID_EP_IDENTIFY = 0x090C,  /**< TID Endpoint Identify */

  TID_DG_SECURITY_EVENT = 0xFF01,  /**< TID Diagnostics Security Event */
  TID_DG_MESSAGE = 0xFF02,  /**< TID Diagnostics Message */
  TID_DG_LEVEL_FOLDBACK = 0xFF03,  /**< TID Diagnostics Level Foldback */
};
/**
 * @}
 */
}  // namespace signet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SIGNET_SIGNETTIDS_H_
