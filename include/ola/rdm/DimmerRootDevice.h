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
 * DimmerRootDevice.h
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file DimmerRootDevice.h
 * @brief Soft responder that implements the Root device in the
 * DimmerResponder.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_DIMMERROOTDEVICE_H_
#define INCLUDE_OLA_RDM_DIMMERROOTDEVICE_H_

#include <ola/rdm/DimmerSubDevice.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/ResponderOps.h>
#include <ola/rdm/UID.h>

#include <string>
#include <map>

namespace ola {
namespace rdm {

/**
 * The root device in the simulated dimmer.
 */
class DimmerRootDevice: public RDMControllerInterface {
 public:
    typedef const std::map<uint16_t, class DimmerSubDevice*> SubDeviceMap;

    DimmerRootDevice(const UID &uid, SubDeviceMap sub_devices);

    void SendRDMRequest(RDMRequest *request, RDMCallback *callback);

 private:
    /**
     * The RDM Operations for the DimmerRootDevice.
     */
    class RDMOps : public ResponderOps<DimmerRootDevice> {
     public:
      static RDMOps *Instance() {
        if (!instance)
          instance = new RDMOps();
        return instance;
      }

     private:
      RDMOps() : ResponderOps<DimmerRootDevice>(PARAM_HANDLERS) {}
      static RDMOps *instance;
    };

    const UID m_uid;
    bool m_identify_on;
    rdm_identify_mode m_identify_mode;
    SubDeviceMap m_sub_devices;

    RDMResponse *GetDeviceInfo(const RDMRequest *request);
    RDMResponse *GetProductDetailList(const RDMRequest *request);
    RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
    RDMResponse *GetManufacturerLabel(const RDMRequest *request);
    RDMResponse *GetDeviceLabel(const RDMRequest *request);
    RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);
    RDMResponse *GetIdentify(const RDMRequest *request);
    RDMResponse *SetIdentify(const RDMRequest *request);
    RDMResponse *GetDmxBlockAddress(const RDMRequest *request);
    RDMResponse *SetDmxBlockAddress(const RDMRequest *request);
    RDMResponse *GetIdentifyMode(const RDMRequest *request);
    RDMResponse *SetIdentifyMode(const RDMRequest *request);

    static const ResponderOps<DimmerRootDevice>::ParamHandler PARAM_HANDLERS[];
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_DIMMERROOTDEVICE_H_
