/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * DimmerRootDevice_h
 * Copyright (C) 2009 Simon Newton
 */

#ifndef PLUGINS_DUMMY_DIMMERROOTDEVICE_H_
#define PLUGINS_DUMMY_DIMMERROOTDEVICE_H_

#include <string>
#include <map>
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderOps.h"
#include "ola/rdm/UID.h"
#include "plugins/dummy/DimmerSubDevice.h"

namespace ola {
namespace plugin {
namespace dummy {

using ola::rdm::RDMResponse;
using std::vector;

class DimmerRootDevice: public ola::rdm::RDMControllerInterface {
  public:
    typedef const map<uint16_t, DimmerSubDevice*> SubDeviceMap;

    DimmerRootDevice(const ola::rdm::UID &uid, SubDeviceMap sub_devices);

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);

  private:
    /**
     * The RDM Operations for the DimmerRootDevice.
     */
    class RDMOps : public ola::rdm::ResponderOps<DimmerRootDevice> {
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

    const ola::rdm::UID m_uid;
    uint8_t m_identify_mode;
    SubDeviceMap m_sub_devices;

    RDMResponse *GetDeviceInfo(const ola::rdm::RDMRequest *request);
    RDMResponse *GetProductDetailList(const ola::rdm::RDMRequest *request);
    RDMResponse *GetDeviceModelDescription(const ola::rdm::RDMRequest *request);
    RDMResponse *GetManufacturerLabel(const ola::rdm::RDMRequest *request);
    RDMResponse *GetDeviceLabel(const ola::rdm::RDMRequest *request);
    RDMResponse *GetSoftwareVersionLabel(const ola::rdm::RDMRequest *request);
    RDMResponse *GetIdentify(const ola::rdm::RDMRequest *request);
    RDMResponse *SetIdentify(const ola::rdm::RDMRequest *request);

    RDMResponse *HandleStringResponse(
        const ola::rdm::RDMRequest *request,
        const std::string &value);

    static const ola::rdm::ResponderOps<DimmerRootDevice>::ParamHandler
      PARAM_HANDLERS[];
};
}  // namespace dummy
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_DUMMY_DIMMERROOTDEVICE_H_
