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
 * DimmerRootDevice.cpp
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
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "plugins/dummy/DimmerRootDevice.h"

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

DimmerRootDevice::RDMOps *DimmerRootDevice::RDMOps::instance = NULL;

const ola::rdm::ResponderOps<DimmerRootDevice>::ParamHandler
    DimmerRootDevice::PARAM_HANDLERS[] = {
  { ola::rdm::PID_DEVICE_INFO,
    &DimmerRootDevice::GetDeviceInfo,
    NULL},
  { ola::rdm::PID_PRODUCT_DETAIL_ID_LIST,
    &DimmerRootDevice::GetProductDetailList,
    NULL},
  { ola::rdm::PID_DEVICE_MODEL_DESCRIPTION,
    &DimmerRootDevice::GetDeviceModelDescription,
    NULL},
  { ola::rdm::PID_MANUFACTURER_LABEL,
    &DimmerRootDevice::GetManufacturerLabel,
    NULL},
  { ola::rdm::PID_DEVICE_LABEL,
    &DimmerRootDevice::GetDeviceLabel,
    NULL},
  { ola::rdm::PID_SOFTWARE_VERSION_LABEL,
    &DimmerRootDevice::GetSoftwareVersionLabel,
    NULL},
  { ola::rdm::PID_IDENTIFY_DEVICE,
    &DimmerRootDevice::GetIdentify,
    &DimmerRootDevice::SetIdentify},
  { 0, NULL, NULL},
};

/**
 * Create a new dimmer root device. Ownership of the DimmerSubDevices is not
 * transferred.
 */
DimmerRootDevice::DimmerRootDevice(const ola::rdm::UID &uid,
                                   SubDeviceMap sub_devices)
    : m_uid(uid),
      m_identify_mode(0),
      m_sub_devices(sub_devices) {
  if (m_sub_devices.size() > ola::rdm::MAX_SUBDEVICE_NUMBER) {
    OLA_FATAL << "More than " << ola::rdm::MAX_SUBDEVICE_NUMBER
              << " sub devices created for device " << uid;
  }
}

/*
 * Handle an RDM Request
 */
void DimmerRootDevice::SendRDMRequest(const ola::rdm::RDMRequest *request,
                                    ola::rdm::RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ola::rdm::ROOT_RDM_DEVICE,
                                       request, callback);
}

RDMResponse *DimmerRootDevice::GetDeviceInfo(
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
  device_info.model = HostToNetwork(
      static_cast<uint16_t>(ola::rdm::OLA_DUMMY_DIMMER_MODEL));
  device_info.product_category = HostToNetwork(
      static_cast<uint16_t>(ola::rdm::PRODUCT_CATEGORY_DIMMER));
  device_info.software_version = HostToNetwork(static_cast<uint32_t>(1));
  device_info.dmx_footprint = 0;
  device_info.current_personality = 1;
  device_info.personality_count = 1;
  device_info.dmx_start_address = 0;
  device_info.sub_device_count = HostToNetwork(
      static_cast<uint16_t>(m_sub_devices.size()));
  device_info.sensor_count = 0;
  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&device_info),
      sizeof(device_info));
}

RDMResponse *DimmerRootDevice::GetProductDetailList(
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

RDMResponse *DimmerRootDevice::GetDeviceModelDescription(
    const ola::rdm::RDMRequest *request) {
  return HandleStringResponse(request, "OLA Dimmer");
}

RDMResponse *DimmerRootDevice::GetManufacturerLabel(
    const ola::rdm::RDMRequest *request) {
  return HandleStringResponse(request, "Open Lighting Project");
}

RDMResponse *DimmerRootDevice::GetDeviceLabel(
    const ola::rdm::RDMRequest *request) {
  return HandleStringResponse(request, "Dummy Dimmer");
}

RDMResponse *DimmerRootDevice::GetSoftwareVersionLabel(
    const ola::rdm::RDMRequest *request) {
  return HandleStringResponse(request, string("OLA Version ") + VERSION);
}

RDMResponse *DimmerRootDevice::GetIdentify(
    const ola::rdm::RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }
  return GetResponseFromData(request, &m_identify_mode,
                             sizeof(m_identify_mode));
}

RDMResponse *DimmerRootDevice::SetIdentify(
    const ola::rdm::RDMRequest *request) {
  uint8_t mode;
  if (request->ParamDataSize() != sizeof(mode)) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  mode = *request->ParamData();
  if (mode == 0 || mode == 1) {
    m_identify_mode = mode;
    OLA_INFO << "Dimmer Root Device " << m_uid << ", identify mode "
             << (m_identify_mode ? "on" : "off");
    return new ola::rdm::RDMSetResponse(
        request->DestinationUID(), request->SourceUID(),
        request->TransactionNumber(), ola::rdm::RDM_ACK,
        0, request->SubDevice(), request->ParamId(),
        NULL, 0);
  } else {
    return NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
  }
}

/*
 * Handle a request that returns a string
 */
RDMResponse *DimmerRootDevice::HandleStringResponse(
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
