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
 * MovingLightResponder_h
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_MOVINGLIGHTRESPONDER_H_
#define INCLUDE_OLA_RDM_MOVINGLIGHTRESPONDER_H_

#include <string>
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderOps.h"
#include "ola/rdm/UID.h"

namespace ola {
namespace rdm {

/**
 * A simulated moving light.
 */
class MovingLightResponder: public RDMControllerInterface {
  public:
    explicit MovingLightResponder(const UID &uid);

    void SendRDMRequest(const RDMRequest *request, RDMCallback *callback);

    uint16_t StartAddress() const { return m_start_address; }
    uint16_t Footprint() const {
      return PERSONALITIES[m_personality].footprint;
    }

  private:
    /**
     * The RDM Operations for the MovingLightResponder.
     */
    class RDMOps : public ResponderOps<MovingLightResponder> {
      public:
        static RDMOps *Instance() {
          if (!instance)
            instance = new RDMOps();
          return instance;
        }

      private:
        RDMOps() : ResponderOps(PARAM_HANDLERS) {}

        static RDMOps *instance;
    };

    const UID m_uid;
    uint16_t m_start_address;
    uint8_t m_personality;
    bool m_identify_mode;
    bool m_pan_invert;
    bool m_tilt_invert;
    uint32_t m_lamp_strikes;

    RDMResponse *GetParamDescription(const RDMRequest *request);
    RDMResponse *GetDeviceInfo(const RDMRequest *request);
    RDMResponse *GetFactoryDefaults(const RDMRequest *request);
    RDMResponse *SetFactoryDefaults(const RDMRequest *request);
    RDMResponse *GetProductDetailList(const RDMRequest *request);
    RDMResponse *GetPersonality(const RDMRequest *request);
    RDMResponse *SetPersonality(const RDMRequest *request);
    RDMResponse *GetPersonalityDescription(const RDMRequest *request);
    RDMResponse *GetDmxStartAddress(const RDMRequest *request);
    RDMResponse *SetDmxStartAddress(const RDMRequest *request);
    RDMResponse *GetLampStrikes(const RDMRequest *request);
    RDMResponse *SetLampStrikes(const RDMRequest *request);
    RDMResponse *GetIdentify(const RDMRequest *request);
    RDMResponse *SetIdentify(const RDMRequest *request);
    RDMResponse *GetPanInvert(const RDMRequest *request);
    RDMResponse *SetPanInvert(const RDMRequest *request);
    RDMResponse *GetTiltInvert(const RDMRequest *request);
    RDMResponse *SetTiltInvert(const RDMRequest *request);
    RDMResponse *GetRealTimeClock(const RDMRequest *request);
    RDMResponse *GetManufacturerLabel(const RDMRequest *request);
    RDMResponse *GetDeviceLabel(const RDMRequest *request);
    RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
    RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);
    RDMResponse *GetOlaCodeVersion(const RDMRequest *request);

    typedef struct {
      uint16_t footprint;
      const char *description;
    } personality_info;

    static const ResponderOps<MovingLightResponder>::ParamHandler
      PARAM_HANDLERS[];
    static const personality_info PERSONALITIES[];
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_MOVINGLIGHTRESPONDER_H_
