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
 * DimmerSubDevice.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include "ola/rdm/DimmerSubDevice.h"

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <vector>

#include "ola/base/Array.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderHelper.h"

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using std::string;
using std::vector;

DimmerSubDevice::RDMOps *DimmerSubDevice::RDMOps::instance = NULL;

const DimmerSubDevice::Personalities *
    DimmerSubDevice::Personalities::Instance() {
  if (!instance) {
    PersonalityList personalities;
    personalities.push_back(Personality(1, "8 bit dimming"));
    personalities.push_back(Personality(2, "16 bit dimming"));
    instance = new Personalities(personalities);
  }
  return instance;
}

DimmerSubDevice::Personalities *DimmerSubDevice::Personalities::instance = NULL;

const ResponderOps<DimmerSubDevice>::ParamHandler
    DimmerSubDevice::PARAM_HANDLERS[] = {
  { PID_DEVICE_INFO,
    &DimmerSubDevice::GetDeviceInfo,
    NULL},
  { PID_PRODUCT_DETAIL_ID_LIST,
    &DimmerSubDevice::GetProductDetailList,
    NULL},
  { PID_DEVICE_MODEL_DESCRIPTION,
    &DimmerSubDevice::GetDeviceModelDescription,
    NULL},
  { PID_MANUFACTURER_LABEL,
    &DimmerSubDevice::GetManufacturerLabel,
    NULL},
  { PID_DEVICE_LABEL,
    &DimmerSubDevice::GetDeviceLabel,
    NULL},
  { PID_SOFTWARE_VERSION_LABEL,
    &DimmerSubDevice::GetSoftwareVersionLabel,
    NULL},
  { PID_DMX_PERSONALITY,
    &DimmerSubDevice::GetPersonality,
    &DimmerSubDevice::SetPersonality},
  { PID_DMX_PERSONALITY_DESCRIPTION,
    &DimmerSubDevice::GetPersonalityDescription,
    NULL},
  { PID_DMX_START_ADDRESS,
    &DimmerSubDevice::GetDmxStartAddress,
    &DimmerSubDevice::SetDmxStartAddress},
  { PID_IDENTIFY_DEVICE,
    &DimmerSubDevice::GetIdentify,
    &DimmerSubDevice::SetIdentify},
  { PID_IDENTIFY_MODE,
    &DimmerSubDevice::GetIdentifyMode,
    &DimmerSubDevice::SetIdentifyMode},
  { 0, NULL, NULL},
};

DimmerSubDevice::DimmerSubDevice(const UID &uid,
                                 uint16_t sub_device_number,
                                 uint16_t sub_device_count)
    : m_uid(uid),
      m_sub_device_number(sub_device_number),
      m_sub_device_count(sub_device_count),
      m_start_address(sub_device_number),
      m_identify_on(false),
      m_identify_mode(IDENTIFY_MODE_LOUD),
      m_personality_manager(Personalities::Instance()) {
}

/*
 * Handle an RDM Request
 */
void DimmerSubDevice::SendRDMRequest(RDMRequest *request,
                                     RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, m_sub_device_number,
                                       request, callback);
}

RDMResponse *DimmerSubDevice::GetDeviceInfo(const RDMRequest *request) {
  return ResponderHelper::GetDeviceInfo(
      request, OLA_DUMMY_DIMMER_MODEL,
      PRODUCT_CATEGORY_DIMMER, 1,
      &m_personality_manager,
      m_start_address,
      m_sub_device_count, 0);
}


bool DimmerSubDevice::SetDmxStartAddress(uint16_t start_address) {
  if (start_address < 1 || start_address + Footprint() - 1 > DMX_UNIVERSE_SIZE)
    return false;

  m_start_address = start_address;
  return true;
}

RDMResponse *DimmerSubDevice::GetProductDetailList(
    const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(request,
    vector<rdm_product_detail> (1, PRODUCT_DETAIL_TEST));
}

RDMResponse *DimmerSubDevice::GetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonality(request, &m_personality_manager);
}

RDMResponse *DimmerSubDevice::SetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::SetPersonality(request, &m_personality_manager,
                                         m_start_address);
}

RDMResponse *DimmerSubDevice::GetPersonalityDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonalityDescription(
      request, &m_personality_manager);
}

RDMResponse *DimmerSubDevice::GetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt16Value(request, m_start_address);
}

RDMResponse *DimmerSubDevice::SetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::SetDmxAddress(request, &m_personality_manager,
                                        &m_start_address);
}

RDMResponse *DimmerSubDevice::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA Dimmer");
}

RDMResponse *DimmerSubDevice::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL);
}

RDMResponse *DimmerSubDevice::GetDeviceLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Dummy Dimmer");
}

RDMResponse *DimmerSubDevice::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

RDMResponse *DimmerSubDevice::GetIdentify(const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_on);
}

RDMResponse *DimmerSubDevice::SetIdentify(const RDMRequest *request) {
  bool old_value = m_identify_on;
  RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_on);
  if (m_identify_on != old_value) {
    OLA_INFO << "Dummy dimmer device " << m_uid << ":" << m_sub_device_number
             << ", identify mode " << (m_identify_on ? "on" : "off");
  }
  return response;
}

RDMResponse *DimmerSubDevice::GetIdentifyMode(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt8Value(request, m_identify_mode);
}

RDMResponse *DimmerSubDevice::SetIdentifyMode(
    const RDMRequest *request) {
  uint8_t new_identify_mode;

  if (!ResponderHelper::ExtractUInt8(request, &new_identify_mode))
    return NackWithReason(request, NR_FORMAT_ERROR);

  if (new_identify_mode != IDENTIFY_MODE_QUIET &&
      new_identify_mode != IDENTIFY_MODE_LOUD)
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);

  m_identify_mode = new_identify_mode;

  return GetResponseFromData(request, NULL, 0);
}
}  // namespace rdm
}  // namespace ola
