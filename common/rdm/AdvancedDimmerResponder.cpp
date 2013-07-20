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
 * AdvancedDimmerResponder.cpp
 * Copyright (C) 2013 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/AdvancedDimmerResponder.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderHelper.h"

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using std::string;
using std::vector;

const AdvancedDimmerResponder::Personalities *
    AdvancedDimmerResponder::Personalities::Instance() {
  if (!instance) {
    PersonalityList personalities;
    personalities.push_back(new Personality(6, "6-Channel"));
    instance = new Personalities(personalities);
  }
  return instance;
}

AdvancedDimmerResponder::Personalities *
    AdvancedDimmerResponder::Personalities::instance = NULL;

AdvancedDimmerResponder::RDMOps *AdvancedDimmerResponder::RDMOps::instance =
  NULL;

const ResponderOps<AdvancedDimmerResponder>::ParamHandler
    AdvancedDimmerResponder::PARAM_HANDLERS[] = {
  { PID_DEVICE_INFO,
    &AdvancedDimmerResponder::GetDeviceInfo,
    NULL},
  { PID_PRODUCT_DETAIL_ID_LIST,
    &AdvancedDimmerResponder::GetProductDetailList,
    NULL},
  { PID_DEVICE_MODEL_DESCRIPTION,
    &AdvancedDimmerResponder::GetDeviceModelDescription,
    NULL},
  { PID_MANUFACTURER_LABEL,
    &AdvancedDimmerResponder::GetManufacturerLabel,
    NULL},
  { PID_DEVICE_LABEL,
    &AdvancedDimmerResponder::GetDeviceLabel,
    NULL},
  { PID_SOFTWARE_VERSION_LABEL,
    &AdvancedDimmerResponder::GetSoftwareVersionLabel,
    NULL},
  { PID_DMX_PERSONALITY,
    &AdvancedDimmerResponder::GetPersonality,
    &AdvancedDimmerResponder::SetPersonality},
  { PID_DMX_PERSONALITY_DESCRIPTION,
    &AdvancedDimmerResponder::GetPersonalityDescription,
    NULL},
  { PID_DMX_START_ADDRESS,
    &AdvancedDimmerResponder::GetDmxStartAddress,
    &AdvancedDimmerResponder::SetDmxStartAddress},
  { PID_IDENTIFY_DEVICE,
    &AdvancedDimmerResponder::GetIdentify,
    &AdvancedDimmerResponder::SetIdentify},
  { PID_IDENTIFY_MODE,
    &AdvancedDimmerResponder::GetIdentifyMode,
    &AdvancedDimmerResponder::SetIdentifyMode},
  { 0, NULL, NULL},
};

/**
 * Create a new dimmer root device. Ownership of the DimmerSubDevices is not
 * transferred.
 */
AdvancedDimmerResponder::AdvancedDimmerResponder(const UID &uid)
    : m_uid(uid),
      m_identify_state(false),
      m_start_address(1),
      m_identify_mode(0),
      m_personality_manager(Personalities::Instance()) {
}

/*
 * Handle an RDM Request
 */
void AdvancedDimmerResponder::SendRDMRequest(const RDMRequest *request,
                                             RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ROOT_RDM_DEVICE, request,
                                       callback);
}
const RDMResponse *AdvancedDimmerResponder::GetDeviceInfo(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  return ResponderHelper::GetDeviceInfo(
      request, OLA_E137_DIMMER_MODEL,
      PRODUCT_CATEGORY_DIMMER, 1,
      &m_personality_manager,
      m_start_address,
      0, 0);
}

const RDMResponse *AdvancedDimmerResponder::GetProductDetailList(
    const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(request,
    vector<rdm_product_detail>(1, PRODUCT_DETAIL_TEST));
}

const RDMResponse *AdvancedDimmerResponder::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA E1.37-1 Dimmer");
}

const RDMResponse *AdvancedDimmerResponder::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL);
}

const RDMResponse *AdvancedDimmerResponder::GetDeviceLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Dummy Dimmer");
}

const RDMResponse *AdvancedDimmerResponder::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

const RDMResponse *AdvancedDimmerResponder::GetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonality(request, &m_personality_manager);
}

const RDMResponse *AdvancedDimmerResponder::SetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::SetPersonality(request, &m_personality_manager,
                                         m_start_address);
}

const RDMResponse *AdvancedDimmerResponder::GetPersonalityDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonalityDescription(
      request, &m_personality_manager);
}

const RDMResponse *AdvancedDimmerResponder::GetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::GetDmxAddress(request, &m_personality_manager,
                                        m_start_address);
}

const RDMResponse *AdvancedDimmerResponder::SetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::SetDmxAddress(request, &m_personality_manager,
                                        &m_start_address);
}

const RDMResponse *AdvancedDimmerResponder::GetIdentify(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_state);
}

const RDMResponse *AdvancedDimmerResponder::SetIdentify(
    const RDMRequest *request) {
  bool old_value = m_identify_state;
  const RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_state);
  if (m_identify_state != old_value) {
    OLA_INFO << "E1.37-1 Dimmer Device " << m_uid << ", identify state "
             << (m_identify_state ? "on" : "off");
  }
  return response;
}

const RDMResponse *AdvancedDimmerResponder::GetIdentifyMode(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt8Value(request, m_identify_mode);
}

const RDMResponse *AdvancedDimmerResponder::SetIdentifyMode(
    const RDMRequest *request) {
  uint8_t arg;
  if (!ResponderHelper::ExtractUInt8(request, &arg)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (arg == 0 || arg == 0xff) {
    m_identify_mode = arg;
    return ResponderHelper::EmptySetResponse(request);
  } else {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }
}
}  // namespace rdm
}  // namespace ola
