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
 * GeneralResponder.h
 * Copyright (C) 2025 Peter Newman
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file GeneralResponder.h
 * @brief An RDM responder that supports E1.37-5 PIDs
 * @}
 */

#ifndef INCLUDE_OLA_RDM_GENERALRESPONDER_H_
#define INCLUDE_OLA_RDM_GENERALRESPONDER_H_

#include <ola/DmxBuffer.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/ResponderNSCStatus.h>
#include <ola/rdm/ResponderOps.h>
#include <ola/rdm/ResponderTagSet.h>
#include <ola/rdm/UID.h>

#include <memory>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

/**
 * A responder with E1.37-5 support.
 */
class GeneralResponder: public RDMControllerInterface {
 public:
  explicit GeneralResponder(const UID &uid);
  ~GeneralResponder();

  void SendRDMRequest(RDMRequest *request, RDMCallback *callback);

  void UpdateNSCStats(const DmxBuffer &buffer);

 private:
  /**
   * The RDM Operations for the GeneralResponder.
   */
  class RDMOps : public ResponderOps<GeneralResponder> {
   public:
    static RDMOps *Instance() {
      if (!instance) {
        instance = new RDMOps();
      }
      return instance;
    }

   private:
    RDMOps() : ResponderOps<GeneralResponder>(PARAM_HANDLERS) {}

    static RDMOps *instance;
  };

  const UID m_uid;
  bool m_identify_mode;
  rdm_shipping_lock_state m_shipping_lock;
  NSCStatus m_nsc_status;
  TagSet m_tags;
  uint32_t m_device_unit_number;

  RDMResponse *GetDeviceInfo(const RDMRequest *request);
  RDMResponse *GetProductDetailList(const RDMRequest *request);
  RDMResponse *GetIdentify(const RDMRequest *request);
  RDMResponse *SetIdentify(const RDMRequest *request);
  RDMResponse *GetManufacturerLabel(const RDMRequest *request);
  RDMResponse *GetDeviceLabel(const RDMRequest *request);
  RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
  RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);
  RDMResponse *GetManufacturerURL(const RDMRequest *request);
  RDMResponse *GetProductURL(const RDMRequest *request);
  RDMResponse *GetFirmwareURL(const RDMRequest *request);
  RDMResponse *GetShippingLock(const RDMRequest *request);
  RDMResponse *SetShippingLock(const RDMRequest *request);
  RDMResponse *GetSerialNumber(const RDMRequest *request);
  RDMResponse *GetTestData(const RDMRequest *request);
  RDMResponse *SetTestData(const RDMRequest *request);
  RDMResponse *GetCommsStatusNSC(const RDMRequest *request);
  RDMResponse *SetCommsStatusNSC(const RDMRequest *request);
  RDMResponse *GetListTags(const RDMRequest *request);
  RDMResponse *SetAddTag(const RDMRequest *request);
  RDMResponse *SetRemoveTag(const RDMRequest *request);
  RDMResponse *GetCheckTag(const RDMRequest *request);
  RDMResponse *SetClearTags(const RDMRequest *request);
  RDMResponse *GetDeviceUnitNumber(const RDMRequest *request);
  RDMResponse *SetDeviceUnitNumber(const RDMRequest *request);

  static const ResponderOps<GeneralResponder>::ParamHandler PARAM_HANDLERS[];
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_GENERALRESPONDER_H_
