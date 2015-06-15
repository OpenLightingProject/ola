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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * RDMHelper.h
 * Various misc RDM functions.
 * Copyright (C) 2005 Simon Newton
 */

/**
 * @addtogroup rdm_helpers
 * @{
 * @file RDMHelper.h
 * @brief Various misc RDM functions
 * @}
 */

#ifndef INCLUDE_OLA_RDM_RDMHELPER_H_
#define INCLUDE_OLA_RDM_RDMHELPER_H_

#include <stdint.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <string>

namespace ola {
namespace rdm {

std::string StatusCodeToString(RDMStatusCode status);

/**
 * @deprecated Use StatusCodeToString instead.
 */
inline std::string ResponseCodeToString(RDMStatusCode status) {
  return StatusCodeToString(status);
}

std::string DataTypeToString(uint8_t type);
std::string LampModeToString(uint8_t lamp_mode);
std::string LampStateToString(uint8_t lamp_state);
std::string NackReasonToString(uint16_t reason);
std::string PowerStateToString(uint8_t power_state);
bool UIntToPowerState(uint8_t state, rdm_power_state *power_state);
std::string PrefixToString(uint8_t prefix);
std::string ProductCategoryToString(uint16_t category);
std::string ProductDetailToString(uint16_t detail);
std::string ResetDeviceToString(uint8_t reset_device);
bool UIntToResetDevice(uint8_t state, rdm_reset_device_mode *reset_device);
std::string SensorTypeToString(uint8_t type);
std::string SensorSupportsRecordingToString(uint8_t supports_recording);
std::string SlotInfoToString(uint8_t slot_type, uint16_t slot_label);
std::string StatusMessageIdToString(uint16_t message_id,
                                    int16_t data1,
                                    int16_t data2);
std::string StatusTypeToString(uint8_t status_type);
std::string UnitToString(uint8_t unit);
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RDMHELPER_H_
