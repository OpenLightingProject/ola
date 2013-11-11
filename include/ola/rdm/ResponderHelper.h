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
 * ResponderHelper.h
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file ResponderHelper.h
 * @brief Helper methods for building RDM responders.
 * @}
 */
#ifndef INCLUDE_OLA_RDM_RESPONDERHELPER_H_
#define INCLUDE_OLA_RDM_RESPONDERHELPER_H_

#include <string>
#include <vector>
#include "ola/network/Interface.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/IPV4Address.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/ResponderNetworkController.h"
#include "ola/rdm/ResponderPersonality.h"
#include "ola/rdm/ResponderSensor.h"

namespace ola {
namespace rdm {

using ola::network::Interface;
using ola::network::InterfacePicker;
using ola::network::IPV4Address;

/**
 * Helper methods for building RDM responders. These don't check that the
 * request is for the specified pid, so be sure to get it right!
 */
class ResponderHelper {
  public:
    // Request Parsing methods
    static bool ExtractUInt8(const RDMRequest *request, uint8_t *output);
    static bool ExtractUInt16(const RDMRequest *request, uint16_t *output);
    static bool ExtractUInt32(const RDMRequest *request, uint32_t *output);

    // Response Generation methods
    static const RDMResponse *GetDeviceInfo(
        const RDMRequest *request,
        uint16_t device_model,
        rdm_product_category product_category,
        uint32_t software_version,
        uint16_t dmx_footprint,
        uint8_t current_personality,
        uint8_t personality_count,
        uint16_t dmx_start_address,
        uint16_t sub_device_count,
        uint8_t sensor_count,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetDeviceInfo(
        const RDMRequest *request,
        uint16_t device_model,
        rdm_product_category product_category,
        uint32_t software_version,
        const PersonalityManager *personality_manager,
        uint16_t dmx_start_address,
        uint16_t sub_device_count,
        uint8_t sensor_count,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetProductDetailList(
        const RDMRequest *request,
        const std::vector<rdm_product_detail> &product_details,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetPersonality(
        const RDMRequest *request,
        const PersonalityManager *personality_manager,
        uint8_t queued_message_count = 0);
    static const RDMResponse *SetPersonality(
        const RDMRequest *request,
        PersonalityManager *personality_manager,
        uint16_t start_address,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetPersonalityDescription(
        const RDMRequest *request,
        const PersonalityManager *personality_manager,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetSlotInfo(
        const RDMRequest *request,
        const PersonalityManager *personality_manager,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetSlotDescription(
        const RDMRequest *request,
        const PersonalityManager *personality_manager,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetSlotDefaultValues(
        const RDMRequest *request,
        const PersonalityManager *personality_manager,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetDmxAddress(
        const RDMRequest *request,
        const PersonalityManager *personality_manager,
        uint16_t start_address,
        uint8_t queued_message_count = 0);
    static const RDMResponse *SetDmxAddress(
        const RDMRequest *request,
        const PersonalityManager *personality_manager,
        uint16_t *dmx_address,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetSensorDefinition(
        const RDMRequest *request, const Sensors &sensor_list);

    static const RDMResponse *GetSensorValue(
        const RDMRequest *request, const Sensors &sensor_list);
    static const RDMResponse *SetSensorValue(
        const RDMRequest *request, const Sensors &sensor_list);

    static const RDMResponse *RecordSensor(
        const RDMRequest *request, const Sensors &sensor_list);

    static const RDMResponse *GetListInterfaces(
        const RDMRequest *request,
        const InterfacePicker *picker,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetInterfaceLabel(
        const RDMRequest *request,
        const InterfacePicker *picker,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetInterfaceHardwareAddress(
        const RDMRequest *request,
        const InterfacePicker *picker,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetIPV4CurrentAddress(
        const RDMRequest *request,
        const InterfacePicker *picker,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetIPV4DefaultRoute(
        const RDMRequest *request, GlobalNetworkGetter *dns_getter);

    static const RDMResponse *GetDNSHostname(
        const RDMRequest *request, GlobalNetworkGetter *dns_getter);

    static const RDMResponse *GetDNSDomainName(
        const RDMRequest *request, GlobalNetworkGetter *dns_getter);

    static const RDMResponse *GetDNSNameServer(
        const RDMRequest *request, GlobalNetworkGetter *dns_getter);

    static const RDMResponse *GetRealTimeClock(
        const RDMRequest *request,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetParamDescription(
        const RDMRequest *request,
        uint16_t pid,
        uint8_t pdl_size,
        rdm_data_type data_type,
        rdm_command_class command_class,
        rdm_pid_unit unit,
        rdm_pid_prefix prefix,
        uint32_t min_value,
        uint32_t default_value,
        uint32_t max_value,
        string description,
        uint8_t queued_message_count = 0);
    static const RDMResponse *GetASCIIParamDescription(
        const RDMRequest *request,
        uint16_t pid,
        rdm_command_class command_class,
        string description,
        uint8_t queued_message_count = 0);
    static const RDMResponse *GetBitFieldParamDescription(
        const RDMRequest *request,
        uint16_t pid,
        uint8_t pdl_size,
        rdm_command_class command_class,
        string description,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetIPV4Address(const RDMRequest *request,
                                             const IPV4Address &value,
                                             uint8_t queued_message_count = 0);

    static const RDMResponse *GetString(const RDMRequest *request,
                                        const std::string &value,
                                        uint8_t queued_message_count = 0);

    static const RDMResponse *EmptyGetResponse(
        const RDMRequest *request,
        uint8_t queued_message_count = 0);
    static const RDMResponse *EmptySetResponse(
        const RDMRequest *request,
        uint8_t queued_message_count = 0);

    static const RDMResponse *SetString(const RDMRequest *request,
                                        std::string *value,
                                        uint8_t queued_message_count = 0);
    static const RDMResponse *GetBoolValue(
        const RDMRequest *request, bool value,
        uint8_t queued_message_count = 0);
    static const RDMResponse *SetBoolValue(
        const RDMRequest *request, bool *value,
        uint8_t queued_message_count = 0);

    static const RDMResponse *GetUInt8Value(
        const RDMRequest *request, uint8_t value,
        uint8_t queued_message_count = 0);
    static const RDMResponse *GetUInt16Value(
        const RDMRequest *request, uint16_t value,
        uint8_t queued_message_count = 0);
    static const RDMResponse *GetUInt32Value(
        const RDMRequest *request, uint32_t value,
        uint8_t queued_message_count = 0);
    static const RDMResponse *SetUInt8Value(
        const RDMRequest *request, uint8_t *value,
        uint8_t queued_message_count = 0);
    static const RDMResponse *SetUInt16Value(
        const RDMRequest *request, uint16_t *value,
        uint8_t queued_message_count = 0);
    static const RDMResponse *SetUInt32Value(
        const RDMRequest *request, uint32_t *value,
        uint8_t queued_message_count = 0);

    struct sensor_value_s {
      uint8_t sensor;
      int16_t value;
      int16_t lowest;
      int16_t highest;
      int16_t recorded;
    } __attribute__((packed));
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RESPONDERHELPER_H_
