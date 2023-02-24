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
 * DimmerRootDevice.cpp
 * Copyright (C) 2013 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <iostream>
#include <string>
#include <vector>
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/DimmerRootDevice.h"
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

DimmerRootDevice::RDMOps *DimmerRootDevice::RDMOps::instance = NULL;

const ResponderOps<DimmerRootDevice>::ParamHandler
    DimmerRootDevice::PARAM_HANDLERS[] = {
  { PID_DEVICE_INFO,
    &DimmerRootDevice::GetDeviceInfo,
    NULL},
  { PID_PRODUCT_DETAIL_ID_LIST,
    &DimmerRootDevice::GetProductDetailList,
    NULL},
  { PID_DEVICE_MODEL_DESCRIPTION,
    &DimmerRootDevice::GetDeviceModelDescription,
    NULL},
  { PID_MANUFACTURER_LABEL,
    &DimmerRootDevice::GetManufacturerLabel,
    NULL},
  { PID_DEVICE_LABEL,
    &DimmerRootDevice::GetDeviceLabel,
    NULL},
  { PID_SOFTWARE_VERSION_LABEL,
    &DimmerRootDevice::GetSoftwareVersionLabel,
    NULL},
  { PID_IDENTIFY_DEVICE,
    &DimmerRootDevice::GetIdentify,
    &DimmerRootDevice::SetIdentify},
  { PID_DMX_BLOCK_ADDRESS,
    &DimmerRootDevice::GetDmxBlockAddress,
    &DimmerRootDevice::SetDmxBlockAddress},
  { PID_IDENTIFY_MODE,
    &DimmerRootDevice::GetIdentifyMode,
    &DimmerRootDevice::SetIdentifyMode},
  { 0, NULL, NULL}
};

/**
 * Create a new dimmer root device. Ownership of the DimmerSubDevices is not
 * transferred.
 */
DimmerRootDevice::DimmerRootDevice(const UID &uid, SubDeviceMap sub_devices)
    : m_uid(uid),
      m_identify_on(false),
      m_identify_mode(IDENTIFY_MODE_LOUD),
      m_sub_devices(sub_devices) {
  if (m_sub_devices.size() > MAX_SUBDEVICE_NUMBER) {
    OLA_FATAL << "More than " << MAX_SUBDEVICE_NUMBER
              << " sub devices created for device " << uid;
  }
}

/*
 * Handle an RDM Request
 */
void DimmerRootDevice::SendRDMRequest(RDMRequest *request,
                                      RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ROOT_RDM_DEVICE, request,
                                       callback);
}

RDMResponse *DimmerRootDevice::GetDeviceInfo(const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  return ResponderHelper::GetDeviceInfo(
      request, OLA_DUMMY_DIMMER_MODEL, PRODUCT_CATEGORY_DIMMER, 1, 0,
      1, 1,  // personality
      0xffff,  // start address
      m_sub_devices.size(), 0);
}

RDMResponse *DimmerRootDevice::GetProductDetailList(const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(request,
    vector<rdm_product_detail>(1, PRODUCT_DETAIL_TEST));
}

RDMResponse *DimmerRootDevice::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA Dimmer");
}

RDMResponse *DimmerRootDevice::GetManufacturerLabel(const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL);
}

RDMResponse *DimmerRootDevice::GetDeviceLabel(const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Dummy Dimmer");
}

RDMResponse *DimmerRootDevice::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

RDMResponse *DimmerRootDevice::GetIdentify(const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_on);
}

RDMResponse *DimmerRootDevice::SetIdentify(const RDMRequest *request) {
  bool old_value = m_identify_on;
  RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_on);
  if (m_identify_on != old_value) {
    OLA_INFO << "Dimmer Root Device " << m_uid << ", identify mode "
             << (m_identify_on ? "on" : "off");
  }
  return response;
}

RDMResponse *DimmerRootDevice::GetDmxBlockAddress(const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  PACK(
  struct block_address_pdl {
    uint16_t total_footprint;
    uint16_t base_address;
  });
  STATIC_ASSERT(sizeof(block_address_pdl) == 4);

  block_address_pdl pdl;
  pdl.base_address = 0;
  pdl.total_footprint = 0;
  uint16_t next_address = 0;

  for (SubDeviceMap::const_iterator iter = m_sub_devices.begin();
       iter != m_sub_devices.end();
       ++iter) {
    if (iter->second->Footprint() != 0) {
      if (next_address == iter->second->GetDmxStartAddress()) {
        next_address += iter->second->Footprint();
      } else if (next_address == 0) {
        next_address = iter->second->GetDmxStartAddress() +
            iter->second->Footprint();
        pdl.base_address = iter->second->GetDmxStartAddress();
      } else {
        pdl.base_address = 0xFFFF;
      }
      pdl.total_footprint += iter->second->Footprint();
    }
  }

  pdl.base_address = HostToNetwork(pdl.base_address);
  pdl.total_footprint = HostToNetwork(pdl.total_footprint);
  return GetResponseFromData(request,
                             reinterpret_cast<uint8_t*>(&pdl),
                             sizeof(pdl));
}

RDMResponse *DimmerRootDevice::SetDmxBlockAddress(const RDMRequest *request) {
  uint16_t base_start_address = 0;
  uint16_t total_footprint = 0;

  if (!ResponderHelper::ExtractUInt16(request, &base_start_address)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  for (SubDeviceMap::const_iterator i = m_sub_devices.begin();
       i != m_sub_devices.end();
       ++i) {
    total_footprint += i->second->Footprint();
  }

  if (base_start_address < 1 ||
      base_start_address + total_footprint - 1 > DMX_MAX_SLOT_VALUE) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  for (SubDeviceMap::const_iterator iter = m_sub_devices.begin();
       iter != m_sub_devices.end();
       ++iter) {
    // We don't check here because we already have for every Sub Device
    iter->second->SetDmxStartAddress(base_start_address);
    base_start_address += iter->second->Footprint();
  }

  return GetResponseFromData(request, NULL, 0);
}

RDMResponse *DimmerRootDevice::GetIdentifyMode(const RDMRequest *request) {
  return ResponderHelper::GetUInt8Value(request, m_identify_mode);
}

RDMResponse *DimmerRootDevice::SetIdentifyMode(const RDMRequest *request) {
  uint8_t new_identify_mode;

  if (!ResponderHelper::ExtractUInt8(request, &new_identify_mode))
    return NackWithReason(request, NR_FORMAT_ERROR);

  if (new_identify_mode != IDENTIFY_MODE_QUIET &&
      new_identify_mode != IDENTIFY_MODE_LOUD)
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);

  m_identify_mode = static_cast<rdm_identify_mode>(new_identify_mode);
  return ResponderHelper::EmptySetResponse(request);
}

}  // namespace rdm
}  // namespace ola
