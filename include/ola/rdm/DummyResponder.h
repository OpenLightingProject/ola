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
 * DummyResponder.h
 * Copyright (C) 2009 Simon Newton
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file DummyResponder.h
 * @brief Basic soft RDM responder.
 * @}
 */
#ifndef INCLUDE_OLA_RDM_DUMMYRESPONDER_H_
#define INCLUDE_OLA_RDM_DUMMYRESPONDER_H_

#include <ola/rdm/NetworkManagerInterface.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/ResponderOps.h>
#include <ola/rdm/ResponderPersonality.h>
#include <ola/rdm/ResponderSensor.h>
#include <ola/rdm/UID.h>

#include <memory>
#include <string>

namespace ola {
namespace rdm {

class DummyResponder: public RDMControllerInterface {
 public:
  explicit DummyResponder(const UID &uid);
  ~DummyResponder();

  void SendRDMRequest(RDMRequest *request, RDMCallback *callback);

  uint16_t StartAddress() const { return m_start_address; }
  uint16_t Footprint() const {
    return m_personality_manager.ActivePersonalityFootprint();
  }

 private:
  /**
   * The RDM Operations for the DummyResponder.
   */
  class RDMOps : public ResponderOps<DummyResponder> {
   public:
    static RDMOps *Instance() {
      if (!instance)
        instance = new RDMOps();
      return instance;
    }

   private:
    RDMOps() : ResponderOps<DummyResponder>(PARAM_HANDLERS) {}

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
  uint16_t m_start_address;
  bool m_identify_mode;
  uint32_t m_lamp_strikes;
  PersonalityManager m_personality_manager;
  Sensors m_sensors;
  std::unique_ptr<NetworkManagerInterface> m_network_manager;

  RDMResponse *GetParamDescription(const RDMRequest *request);
  RDMResponse *GetDeviceInfo(const RDMRequest *request);
  RDMResponse *GetFactoryDefaults(const RDMRequest *request);
  RDMResponse *SetFactoryDefaults(const RDMRequest *request);
  RDMResponse *GetProductDetailList(const RDMRequest *request);
  RDMResponse *GetPersonality(const RDMRequest *request);
  RDMResponse *SetPersonality(const RDMRequest *request);
  RDMResponse *GetPersonalityDescription(const RDMRequest *request);
  RDMResponse *GetSlotInfo(const RDMRequest *request);
  RDMResponse *GetSlotDescription(const RDMRequest *request);
  RDMResponse *GetSlotDefaultValues(const RDMRequest *request);
  RDMResponse *GetDmxStartAddress(const RDMRequest *request);
  RDMResponse *SetDmxStartAddress(const RDMRequest *request);
  RDMResponse *GetLampStrikes(const RDMRequest *request);
  RDMResponse *SetLampStrikes(const RDMRequest *request);
  RDMResponse *GetIdentify(const RDMRequest *request);
  RDMResponse *SetIdentify(const RDMRequest *request);
  RDMResponse *GetRealTimeClock(const RDMRequest *request);
  RDMResponse *GetManufacturerLabel(const RDMRequest *request);
  RDMResponse *GetDeviceLabel(const RDMRequest *request);
  RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
  RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);
  RDMResponse *GetOlaCodeVersion(const RDMRequest *request);
  RDMResponse *GetSensorDefinition(const RDMRequest *request);
  RDMResponse *GetSensorValue(const RDMRequest *request);
  RDMResponse *SetSensorValue(const RDMRequest *request);
  RDMResponse *RecordSensor(const RDMRequest *request);
  RDMResponse *GetListInterfaces(const RDMRequest *request);
  RDMResponse *GetInterfaceLabel(const RDMRequest *request);
  RDMResponse *GetInterfaceHardwareAddressType1(
      const RDMRequest *request);
  RDMResponse *GetIPV4CurrentAddress(const RDMRequest *request);
  RDMResponse *GetIPV4DefaultRoute(const RDMRequest *request);
  RDMResponse *GetDNSHostname(const RDMRequest *request);
  RDMResponse *GetDNSDomainName(const RDMRequest *request);
  RDMResponse *GetDNSNameServer(const RDMRequest *request);
  RDMResponse *GetManufacturerURL(const RDMRequest *request);
  RDMResponse *GetProductURL(const RDMRequest *request);
  RDMResponse *GetFirmwareURL(const RDMRequest *request);
  RDMResponse *GetTestData(const RDMRequest *request);
  RDMResponse *SetTestData(const RDMRequest *request);

  static const ResponderOps<DummyResponder>::ParamHandler PARAM_HANDLERS[];
  static const uint8_t DEFAULT_PERSONALITY = 2;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_DUMMYRESPONDER_H_
