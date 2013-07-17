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
  { 0, NULL, NULL},

};

/**
 * Create a new dimmer root device. Ownership of the DimmerSubDevices is not
 * transferred.
 */
DimmerRootDevice::DimmerRootDevice(const UID &uid, SubDeviceMap sub_devices)
    : m_uid(uid),
      m_identify_mode(false),
      m_sub_devices(sub_devices) {
  if (m_sub_devices.size() > MAX_SUBDEVICE_NUMBER) {
    OLA_FATAL << "More than " << MAX_SUBDEVICE_NUMBER
              << " sub devices created for device " << uid;
  }
}

/*
 * Handle an RDM Request
 */
void DimmerRootDevice::SendRDMRequest(const RDMRequest *request,
                                      RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ROOT_RDM_DEVICE, request,
                                       callback);
}

const RDMResponse *DimmerRootDevice::GetDeviceInfo(const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  return ResponderHelper::GetDeviceInfo(
      request, OLA_DUMMY_DIMMER_MODEL, PRODUCT_CATEGORY_DIMMER, 1, 0, 1,
      1, 0, m_sub_devices.size(), 0);
}

const RDMResponse *DimmerRootDevice::GetProductDetailList(
    const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(request,
    vector<rdm_product_detail>(1, PRODUCT_DETAIL_TEST));
}

const RDMResponse *DimmerRootDevice::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA Dimmer");
}

const RDMResponse *DimmerRootDevice::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL);
}

const RDMResponse *DimmerRootDevice::GetDeviceLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Dummy Dimmer");
}

const RDMResponse *DimmerRootDevice::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

const RDMResponse *DimmerRootDevice::GetIdentify(const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_mode);
}

const RDMResponse *DimmerRootDevice::SetIdentify(const RDMRequest *request) {
  bool old_value = m_identify_mode;
  const RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_mode);
  if (m_identify_mode != old_value) {
    OLA_INFO << "Dimmer Root Device " << m_uid << ", identify mode "
             << (m_identify_mode ? "on" : "off");
  }
  return response;
}

const RDMResponse *DimmerRootDevice::GetDmxBlockAddress(
    const RDMRequest *request){

  struct block_address_pdl{
    uint16_t total_footprint;
    uint16_t base_address;
  };

  SubDeviceMap::const_iterator iter = m_sub_devices.begin();
  block_address_pdl pdl;
  pdl.base_address = iter->second->GetDmxStartAddress();
  pdl.total_footprint = iter->second->Footprint();
  uint16_t next_address = pdl.base_address + pdl.total_footprint;

  for(++iter; iter != m_sub_devices.end(); ++iter) {
    if(iter->second->Footprint()) {
      if(pdl.base_address != 0xFFFF &&
          (iter->second->GetDmxStartAddress() < pdl.base_address ||
          iter->second->GetDmxStartAddress() != next_address)) {
        pdl.base_address = 0xFFFF; // NEEDS TO BE A CONSTANT SOMEWHERE
      }
    }

    pdl.total_footprint += iter->second->Footprint();
    next_address += iter->second->Footprint();
  }

  return GetResponseFromData(request,
                             reinterpret_cast<uint8_t*>(&pdl),
                             sizeof(pdl));
}

const RDMResponse *DimmerRootDevice::SetDmxBlockAddress(
    const RDMRequest *request){
  uint16_t base_start_address = 0;
  uint16_t total_footprint = 0;

  if(!ResponderHelper::ExtractUInt16(request, &base_start_address)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  for(SubDeviceMap::const_iterator i = m_sub_devices.begin();
      i != m_sub_devices.end();
      ++i) {
    total_footprint += i->second->Footprint();
  }

  if(base_start_address < 1 ||
      base_start_address + total_footprint > DMX_MAX_CHANNEL_VALUE) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

   for(SubDeviceMap::const_iterator iter = m_sub_devices.begin();
       iter != m_sub_devices.end();
       ++iter) {
     // We don't check here because we already have for every Sub Device
     iter->second->SetDmxStartAddress(base_start_address);
     base_start_address += iter->second->Footprint();
  }

  return GetResponseFromData(request, NULL, 0);
}

}  // namespace rdm
}  // namespace ola
