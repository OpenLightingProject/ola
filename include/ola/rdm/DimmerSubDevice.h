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
 * DimmerSubDevice.h
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file DimmerSubDevice.h
 * @brief A soft responder that implements a SubDevice in DimmerResponder
 * @}
 */

#ifndef INCLUDE_OLA_RDM_DIMMERSUBDEVICE_H_
#define INCLUDE_OLA_RDM_DIMMERSUBDEVICE_H_

#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/ResponderOps.h>
#include <ola/rdm/ResponderPersonality.h>
#include <ola/rdm/UID.h>

#include <string>

namespace ola {
namespace rdm {

/**
 * A sub device in the simulated dimmer.
 */
class DimmerSubDevice: public RDMControllerInterface {
 public:
  /**
   * We need the total sub device count here because the sub device field in
   * DEVICE_INFO must be the same for both the root and all sub devices
   * (10.5).
   */
  DimmerSubDevice(const UID &uid, uint16_t sub_device_number,
                  uint16_t total_sub_devices);

  void SendRDMRequest(RDMRequest *request, RDMCallback *callback);

  uint16_t Footprint() const {
    return m_personality_manager.ActivePersonalityFootprint();
  }

  bool SetDmxStartAddress(uint16_t start_address);

  uint16_t GetDmxStartAddress() const {
     return m_start_address;
  }

 private:
  /**
   * The RDM Operations for the DimmerSubDevice.
   */
  class RDMOps : public ResponderOps<DimmerSubDevice> {
   public:
    static RDMOps *Instance() {
      if (!instance)
        instance = new RDMOps();
      return instance;
    }

   private:
    RDMOps() : ResponderOps<DimmerSubDevice>(PARAM_HANDLERS, true) {}

    static RDMOps *instance;
  };

  /**
   * The personalities
   */
  class Personalities : public PersonalityCollection {
   public:
    static const Personalities *Instance();

   private:
    explicit Personalities(const PersonalityList &personalities) :
      PersonalityCollection(personalities) {
    }

    static Personalities *instance;
  };

  const UID m_uid;
  const uint16_t m_sub_device_number;
  const uint16_t m_sub_device_count;
  uint16_t m_start_address;
  bool m_identify_on;
  uint8_t m_identify_mode;
  PersonalityManager m_personality_manager;

  RDMResponse *GetDeviceInfo(const RDMRequest *request);
  RDMResponse *GetProductDetailList(const RDMRequest *request);
  RDMResponse *GetPersonality(const RDMRequest *request);
  RDMResponse *SetPersonality(const RDMRequest *request);
  RDMResponse *GetPersonalityDescription(const RDMRequest *request);
  RDMResponse *GetDmxStartAddress(const RDMRequest *request);
  RDMResponse *SetDmxStartAddress(const RDMRequest *request);
  RDMResponse *GetIdentify(const RDMRequest *request);
  RDMResponse *SetIdentify(const RDMRequest *request);
  RDMResponse *SetIdentifyMode(const RDMRequest *request);
  RDMResponse *GetIdentifyMode(const RDMRequest *request);
  RDMResponse *GetRealTimeClock(const RDMRequest *request);
  RDMResponse *GetManufacturerLabel(const RDMRequest *request);
  RDMResponse *GetDeviceLabel(const RDMRequest *request);
  RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
  RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);

  static const ResponderOps<DimmerSubDevice>::ParamHandler PARAM_HANDLERS[];
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_DIMMERSUBDEVICE_H_
