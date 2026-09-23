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
 * GeneralResponder.cpp
 * Copyright (C) 2025 Peter Newman
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <iostream>
#include <string>
#include <vector>
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/strings/Format.h"
#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/GeneralResponder.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/stl/STLUtils.h"

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using std::string;
using std::vector;
using std::auto_ptr;

GeneralResponder::RDMOps *GeneralResponder::RDMOps::instance = NULL;

const ResponderOps<GeneralResponder>::ParamHandler
    GeneralResponder::PARAM_HANDLERS[] = {
  { PID_DEVICE_INFO,
    &GeneralResponder::GetDeviceInfo,
    NULL},
  { PID_PRODUCT_DETAIL_ID_LIST,
    &GeneralResponder::GetProductDetailList,
    NULL},
  { PID_DEVICE_MODEL_DESCRIPTION,
    &GeneralResponder::GetDeviceModelDescription,
    NULL},
  { PID_MANUFACTURER_LABEL,
    &GeneralResponder::GetManufacturerLabel,
    NULL},
  { PID_DEVICE_LABEL,
    &GeneralResponder::GetDeviceLabel,
    NULL},
  { PID_SOFTWARE_VERSION_LABEL,
    &GeneralResponder::GetSoftwareVersionLabel,
    NULL},
  { PID_IDENTIFY_DEVICE,
    &GeneralResponder::GetIdentify,
    &GeneralResponder::SetIdentify},
  { PID_MANUFACTURER_URL,
    &GeneralResponder::GetManufacturerURL,
    NULL},
  { PID_PRODUCT_URL,
    &GeneralResponder::GetProductURL,
    NULL},
  { PID_FIRMWARE_URL,
    &GeneralResponder::GetFirmwareURL,
    NULL},
  { PID_SHIPPING_LOCK,
    &GeneralResponder::GetShippingLock,
    &GeneralResponder::SetShippingLock},
  { PID_SERIAL_NUMBER,
    &GeneralResponder::GetSerialNumber,
    NULL},
  { PID_TEST_DATA,
    &GeneralResponder::GetTestData,
    &GeneralResponder::SetTestData},
  { PID_COMMS_STATUS_NSC,
    &GeneralResponder::GetCommsStatusNSC,
    &GeneralResponder::SetCommsStatusNSC},
  { PID_LIST_TAGS,
    &GeneralResponder::GetListTags,
    NULL},
  { PID_ADD_TAG,
    NULL,
    &GeneralResponder::SetAddTag},
  { PID_REMOVE_TAG,
    NULL,
    &GeneralResponder::SetRemoveTag},
  { PID_CHECK_TAG,
    &GeneralResponder::GetCheckTag,
    NULL},
  { PID_CLEAR_TAGS,
    NULL,
    &GeneralResponder::SetClearTags},
  { PID_DEVICE_UNIT_NUMBER,
    &GeneralResponder::GetDeviceUnitNumber,
    &GeneralResponder::SetDeviceUnitNumber},
  { 0, NULL, NULL},
};


/**
 * New GeneralResponder
 */
GeneralResponder::GeneralResponder(const UID &uid)
    : m_uid(uid),
      m_identify_mode(false),
      m_shipping_lock(SHIPPING_LOCK_STATE_PARTIALLY_LOCKED),
      m_nsc_status(NSCStatus::NSCStatusOptions()),
      m_device_unit_number(0) {
}


GeneralResponder::~GeneralResponder() {
  m_tags.Clear();
}

/*
 * Handle an RDM Request
 */
void GeneralResponder::SendRDMRequest(RDMRequest *request,
                                      RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ROOT_RDM_DEVICE, request,
                                       callback);
}

void GeneralResponder::UpdateNSCStats(const DmxBuffer &buffer) {
  m_nsc_status.UpdateStats(buffer);
}

RDMResponse *GeneralResponder::GetDeviceInfo(
    const RDMRequest *request) {
  return ResponderHelper::GetDeviceInfo(
      request, OLA_E137_5_MODEL, PRODUCT_CATEGORY_TEST,
      1, 0, 1, 1, ZERO_FOOTPRINT_DMX_ADDRESS, 0, 0);
}

RDMResponse *GeneralResponder::GetProductDetailList(
    const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(
      request, vector<rdm_product_detail>(1, PRODUCT_DETAIL_TEST));
}

RDMResponse *GeneralResponder::GetIdentify(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_mode);
}

RDMResponse *GeneralResponder::SetIdentify(
    const RDMRequest *request) {
  bool old_value = m_identify_mode;
  RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_mode);
  if (m_identify_mode != old_value) {
    OLA_INFO << "General Device " << m_uid << ", identify mode "
             << (m_identify_mode ? "on" : "off");
  }
  return response;
}

RDMResponse *GeneralResponder::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA General Parameter Device");
}

RDMResponse *GeneralResponder::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL);
}

RDMResponse *GeneralResponder::GetDeviceLabel(const RDMRequest *request) {
  return ResponderHelper::GetString(request, "General Parameter Device");
}

RDMResponse *GeneralResponder::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

RDMResponse *GeneralResponder::GetManufacturerURL(
    const RDMRequest *request) {
  return ResponderHelper::GetString(
      // TODO(Peter): This field's length isn't limited in the spec
      request, OLA_MANUFACTURER_URL, 0, UINT8_MAX);
}

RDMResponse *GeneralResponder::GetProductURL(
    const RDMRequest *request) {
  return ResponderHelper::GetString(
      request,
      "https://openlighting.org/rdm-tools/dummy-responders/",
      0,
      UINT8_MAX); // TODO(Peter): This field's length isn't limited in the spec
}

RDMResponse *GeneralResponder::GetFirmwareURL(
    const RDMRequest *request) {
  return ResponderHelper::GetString(
      request,
      "https://github.com/OpenLightingProject/ola",
      0,
      UINT8_MAX); // TODO(Peter): This field's length isn't limited in the spec
}

RDMResponse *GeneralResponder::GetShippingLock(
    const RDMRequest *request) {
  uint8_t value = m_shipping_lock;
  return ResponderHelper::GetUInt8Value(request, value);
}

RDMResponse *GeneralResponder::SetShippingLock(
    const RDMRequest *request) {
  uint8_t new_value;
  if (!ResponderHelper::ExtractUInt8(request, &new_value)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  // SHIPPING_LOCK_STATE_PARTIALLY_LOCKED isn't allowed within SET
  if (new_value > static_cast<uint8_t>(SHIPPING_LOCK_STATE_LOCKED)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  m_shipping_lock = static_cast<rdm_shipping_lock_state>(new_value);
  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *GeneralResponder::GetSerialNumber(const RDMRequest *request) {
  // Return the device part of the UID
  std::ostringstream str;
  str << ola::strings::ToHex(m_uid.DeviceId(), false);
  return ResponderHelper::GetString(request,
                                    str.str(),
                                    0,
                                    MAX_RDM_SERIAL_NUMBER_LENGTH);
}

RDMResponse *GeneralResponder::GetTestData(const RDMRequest *request) {
  return ResponderHelper::GetTestData(request);
}

RDMResponse *GeneralResponder::SetTestData(const RDMRequest *request) {
  return ResponderHelper::SetTestData(request);
}

RDMResponse *GeneralResponder::GetCommsStatusNSC(const RDMRequest *request) {
  return ResponderHelper::GetCommsStatusNSC(request, &m_nsc_status);
}

RDMResponse *GeneralResponder::SetCommsStatusNSC(const RDMRequest *request) {
  return ResponderHelper::SetCommsStatusNSC(request, &m_nsc_status);
}

RDMResponse *GeneralResponder::GetListTags(const RDMRequest *request) {
  return ResponderHelper::GetListTags(request, &m_tags);
}

RDMResponse *GeneralResponder::SetAddTag(const RDMRequest *request) {
  return ResponderHelper::SetAddTag(request, &m_tags);
}

RDMResponse *GeneralResponder::SetRemoveTag(const RDMRequest *request) {
  return ResponderHelper::SetRemoveTag(request, &m_tags);
}

RDMResponse *GeneralResponder::GetCheckTag(const RDMRequest *request) {
  return ResponderHelper::GetCheckTag(request, &m_tags);
}

RDMResponse *GeneralResponder::SetClearTags(const RDMRequest *request) {
  return ResponderHelper::SetClearTags(request, &m_tags);
}

RDMResponse *GeneralResponder::GetDeviceUnitNumber(const RDMRequest *request) {
  return ResponderHelper::GetUInt32Value(request, m_device_unit_number);
}

RDMResponse *GeneralResponder::SetDeviceUnitNumber(const RDMRequest *request) {
  return ResponderHelper::SetUInt32Value(request, &m_device_unit_number);
}
}  // namespace rdm
}  // namespace ola
