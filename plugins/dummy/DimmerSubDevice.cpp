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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "plugins/dummy/DimmerSubDevice.h"

namespace ola {
namespace plugin {
namespace dummy {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using ola::rdm::GetResponseFromData;
using ola::rdm::NackWithReason;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::rdm_response_code;
using std::string;
using std::vector;

DimmerSubDevice::RDMOps *DimmerSubDevice::RDMOps::instance = NULL;

const ola::rdm::ResponderOps<DimmerSubDevice>::ParamHandler
    DimmerSubDevice::PARAM_HANDLERS[] = {
  { ola::rdm::PID_DEVICE_INFO,
    &DimmerSubDevice::GetDeviceInfo,
    NULL},
  { ola::rdm::PID_PRODUCT_DETAIL_ID_LIST,
    &DimmerSubDevice::GetProductDetailList,
    NULL},
  { ola::rdm::PID_DEVICE_MODEL_DESCRIPTION,
    &DimmerSubDevice::GetDeviceModelDescription,
    NULL},
  { ola::rdm::PID_MANUFACTURER_LABEL,
    &DimmerSubDevice::GetManufacturerLabel,
    NULL},
  { ola::rdm::PID_DEVICE_LABEL,
    &DimmerSubDevice::GetDeviceLabel,
    NULL},
  { ola::rdm::PID_SOFTWARE_VERSION_LABEL,
    &DimmerSubDevice::GetSoftwareVersionLabel,
    NULL},
  { ola::rdm::PID_DMX_START_ADDRESS,
    &DimmerSubDevice::GetDmxStartAddress,
    &DimmerSubDevice::SetDmxStartAddress},
  { ola::rdm::PID_IDENTIFY_DEVICE,
    &DimmerSubDevice::GetIdentify,
    &DimmerSubDevice::SetIdentify},
  { 0, NULL, NULL},
};

/*
 * Handle an RDM Request
 */
void DimmerSubDevice::SendRDMRequest(const ola::rdm::RDMRequest *request,
                                    ola::rdm::RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, m_sub_device_number,
                                       request, callback);
}

RDMResponse *DimmerSubDevice::GetDeviceInfo(
    const ola::rdm::RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  struct device_info_s {
    uint16_t rdm_version;
    uint16_t model;
    uint16_t product_category;
    uint32_t software_version;
    uint16_t dmx_footprint;
    uint8_t current_personality;
    uint8_t personality_count;
    uint16_t dmx_start_address;
    uint16_t sub_device_count;
    uint8_t sensor_count;
  } __attribute__((packed));

  struct device_info_s device_info;
  device_info.rdm_version = HostToNetwork(static_cast<uint16_t>(0x100));
  device_info.model = HostToNetwork(static_cast<uint16_t>(1));
  device_info.product_category = HostToNetwork(
      static_cast<uint16_t>(ola::rdm::PRODUCT_CATEGORY_DIMMER));
  device_info.software_version = HostToNetwork(
      static_cast<uint32_t>(ola::rdm::OLA_DUMMY_DIMMER_MODEL));
  device_info.dmx_footprint = HostToNetwork(1);
  device_info.current_personality = 1;
  device_info.personality_count = 1;
  device_info.dmx_start_address = HostToNetwork(m_start_address);
  device_info.sub_device_count = 0;
  device_info.sensor_count = 0;
  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&device_info),
      sizeof(device_info));
}

RDMResponse *DimmerSubDevice::GetProductDetailList(
    const ola::rdm::RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  uint16_t product_details[] = {
    ola::rdm::PRODUCT_DETAIL_TEST,
  };

  for (unsigned int i = 0; i < arraysize(product_details); i++)
    product_details[i] = HostToNetwork(product_details[i]);

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&product_details),
      sizeof(product_details));
}

RDMResponse *DimmerSubDevice::GetDmxStartAddress(
    const ola::rdm::RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  uint16_t address = HostToNetwork(m_start_address);
  return GetResponseFromData(
      request,
      reinterpret_cast<const uint8_t*>(&address),
      sizeof(address));
}

RDMResponse *DimmerSubDevice::SetDmxStartAddress(
    const ola::rdm::RDMRequest *request) {
  uint16_t address;
  if (request->ParamDataSize() != sizeof(address)) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&address), request->ParamData(),
         sizeof(address));
  address = NetworkToHost(address);
  if (address == 0 || address > DMX_UNIVERSE_SIZE) {
    return NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
  } else {
    m_start_address = address;
    return new ola::rdm::RDMSetResponse(
        request->DestinationUID(), request->SourceUID(),
        request->TransactionNumber(), ola::rdm::RDM_ACK, 0,
        request->SubDevice(), request->ParamId(), NULL, 0);
  }
}

RDMResponse *DimmerSubDevice::GetDeviceModelDescription(
    const ola::rdm::RDMRequest *request) {
  return HandleStringResponse(request, "OLA Dummy Dimmer ");
}

RDMResponse *DimmerSubDevice::GetManufacturerLabel(
    const ola::rdm::RDMRequest *request) {
  return HandleStringResponse(request, "Open Lighting Project");
}

RDMResponse *DimmerSubDevice::GetDeviceLabel(
    const ola::rdm::RDMRequest *request) {
  return HandleStringResponse(request, "Dummy Dimmer");
}

RDMResponse *DimmerSubDevice::GetSoftwareVersionLabel(
    const ola::rdm::RDMRequest *request) {
  return HandleStringResponse(request, string("OLA Version ") + VERSION);
}

RDMResponse *DimmerSubDevice::GetIdentify(const ola::rdm::RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  return GetResponseFromData(
      request,
      &m_identify_mode,
      sizeof(m_identify_mode));
}

RDMResponse *DimmerSubDevice::SetIdentify(const ola::rdm::RDMRequest *request) {
  uint8_t mode;
  if (request->ParamDataSize() != sizeof(mode)) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  mode = *request->ParamData();
  if (mode == 0 || mode == 1) {
    m_identify_mode = mode;
    OLA_INFO << "Dummy dimmer device " << m_uid << ":" << m_sub_device_number
             << ", identify mode " << (m_identify_mode ? "on" : "off");
    return new ola::rdm::RDMSetResponse(
      request->DestinationUID(),
      request->SourceUID(),
      request->TransactionNumber(),
      ola::rdm::RDM_ACK,
      0,
      request->SubDevice(),
      request->ParamId(),
      NULL,
      0);
  } else {
    return NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
  }
}

/*
 * Handle a request that returns a string
 */
RDMResponse *DimmerSubDevice::HandleStringResponse(
    const ola::rdm::RDMRequest *request,
    const std::string &value) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }
  return GetResponseFromData(
      request,
      reinterpret_cast<const uint8_t*>(value.data()),
      value.size());
}
}  // namespace dummy
}  // namespace plugin
}  // namespace ola
