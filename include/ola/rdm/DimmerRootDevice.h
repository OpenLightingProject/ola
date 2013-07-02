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
 * DimmerRootDevice_h
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_DIMMERROOTDEVICE_H_
#define INCLUDE_OLA_RDM_DIMMERROOTDEVICE_H_

#include <string>
#include <map>
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/ResponderOps.h"
#include "ola/rdm/UID.h"

namespace ola {
namespace rdm {

using std::vector;

/**
 * The root device in the simulated dimmer.
 */
class DimmerRootDevice: public RDMControllerInterface {
  public:
    typedef const map<uint16_t, class DimmerSubDevice*> SubDeviceMap;

    DimmerRootDevice(const UID &uid, SubDeviceMap sub_devices);

    void SendRDMRequest(const RDMRequest *request, RDMCallback *callback);

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
    bool m_identify_mode;
    SubDeviceMap m_sub_devices;

    const RDMResponse *GetDeviceInfo(const RDMRequest *request);
    const RDMResponse *GetProductDetailList(const RDMRequest *request);
    const RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
    const RDMResponse *GetManufacturerLabel(const RDMRequest *request);
    const RDMResponse *GetDeviceLabel(const RDMRequest *request);
    const RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);
    const RDMResponse *GetIdentify(const RDMRequest *request);
    const RDMResponse *SetIdentify(const RDMRequest *request);

    static const ResponderOps<DimmerRootDevice>::ParamHandler PARAM_HANDLERS[];
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_DIMMERROOTDEVICE_H_
