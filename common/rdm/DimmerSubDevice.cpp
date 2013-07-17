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
 * DimmerSubDevice.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/DimmerSubDevice.h"
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
    personalities.push_back(new Personality(1, "8 bit dimming"));
    personalities.push_back(new Personality(2, "16 bit dimming"));
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
  { 0, NULL, NULL},
};

DimmerSubDevice::DimmerSubDevice(const UID &uid, uint16_t sub_device_number)
    : m_uid(uid),
      m_sub_device_number(sub_device_number),
      m_start_address(sub_device_number),
      m_identify_mode(false),
      m_personality_manager(Personalities::Instance()) {
}

/*
 * Handle an RDM Request
 */
void DimmerSubDevice::SendRDMRequest(const RDMRequest *request,
                                     RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, m_sub_device_number,
                                       request, callback);
}

const RDMResponse *DimmerSubDevice::GetDeviceInfo(const RDMRequest *request) {
  return ResponderHelper::GetDeviceInfo(
      request, OLA_DUMMY_DIMMER_MODEL,
      PRODUCT_CATEGORY_DIMMER, 1,
      &m_personality_manager,
      m_start_address,
      0, 0);
}


bool DimmerSubDevice::SetDmxStartAddress(uint16_t start_address) {
  if(start_address < 1 || start_address + Footprint() > DMX_MAX_CHANNEL_VALUE)
    return false;

  m_start_address = start_address;
  return true;
}

const RDMResponse *DimmerSubDevice::GetProductDetailList(
    const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(request,
    std::vector<rdm_product_detail> (1, PRODUCT_DETAIL_TEST));
}

const RDMResponse *DimmerSubDevice::GetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonality(request, &m_personality_manager);
}

const RDMResponse *DimmerSubDevice::SetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::SetPersonality(request, &m_personality_manager,
                                         m_start_address);
}

const RDMResponse *DimmerSubDevice::GetPersonalityDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonalityDescription(
      request, &m_personality_manager);
}

const RDMResponse *DimmerSubDevice::GetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt16Value(request, m_start_address);
}

const RDMResponse *DimmerSubDevice::SetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::SetDmxAddress(request, &m_personality_manager,
                                        &m_start_address);
}

const RDMResponse *DimmerSubDevice::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA Dimmer");
}

const RDMResponse *DimmerSubDevice::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL);
}

const RDMResponse *DimmerSubDevice::GetDeviceLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Dummy Dimmer");
}

const RDMResponse *DimmerSubDevice::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

const RDMResponse *DimmerSubDevice::GetIdentify(const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_mode);
}

const RDMResponse *DimmerSubDevice::SetIdentify(const RDMRequest *request) {
  bool old_value = m_identify_mode;
  const RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_mode);
  if (m_identify_mode != old_value) {
    OLA_INFO << "Dummy dimmer device " << m_uid << ":" << m_sub_device_number
             << ", identify mode " << (m_identify_mode ? "on" : "off");
  }
  return response;
}
}  // namespace rdm
}  // namespace ola
