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
 * DimmerSubDevice.h
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_DIMMERSUBDEVICE_H_
#define INCLUDE_OLA_RDM_DIMMERSUBDEVICE_H_

#include <string>
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderOps.h"
#include "ola/rdm/UID.h"

namespace ola {
namespace rdm {

/**
 * A sub device in the simulated dimmer.
 */
class DimmerSubDevice: public RDMControllerInterface {
  public:
    DimmerSubDevice(const UID &uid, uint16_t sub_device_number)
        : m_uid(uid),
          m_sub_device_number(sub_device_number),
          m_start_address(sub_device_number),
          m_identify_mode(false) {
    }

    void SendRDMRequest(const RDMRequest *request, RDMCallback *callback);

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
        RDMOps() : ResponderOps(PARAM_HANDLERS) {}

        static RDMOps *instance;
    };

    const UID m_uid;
    const uint16_t m_sub_device_number;
    uint16_t m_start_address;
    bool m_identify_mode;

    RDMResponse *GetDeviceInfo(const RDMRequest *request);
    RDMResponse *GetProductDetailList(const RDMRequest *request);
    RDMResponse *GetDmxStartAddress(const RDMRequest *request);
    RDMResponse *SetDmxStartAddress(const RDMRequest *request);
    RDMResponse *GetIdentify(const RDMRequest *request);
    RDMResponse *SetIdentify(const RDMRequest *request);
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
