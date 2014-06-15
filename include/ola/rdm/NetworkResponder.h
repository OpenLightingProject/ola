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
 * NetworkResponder.h
 * Copyright (C) 2013 Peter Newman
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file NetworkResponder.h
 * @brief An RDM responder that supports E1.37-2 PIDs
 * @}
 */

#ifndef INCLUDE_OLA_RDM_NETWORKRESPONDER_H_
#define INCLUDE_OLA_RDM_NETWORKRESPONDER_H_

#include <ola/rdm/NetworkManagerInterface.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/ResponderOps.h>
#include <ola/rdm/UID.h>

#include <memory>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

/**
 * A responder with no footprint and E1.37-2 support.
 */
class NetworkResponder: public RDMControllerInterface {
 public:
  explicit NetworkResponder(const UID &uid);
  ~NetworkResponder();

  void SendRDMRequest(const RDMRequest *request, RDMCallback *callback);

 private:
  /**
   * The RDM Operations for the NetworkResponder.
   */
  class RDMOps : public ResponderOps<NetworkResponder> {
   public:
    static RDMOps *Instance() {
      if (!instance) {
        instance = new RDMOps();
      }
      return instance;
    }

   private:
    RDMOps() : ResponderOps<NetworkResponder>(PARAM_HANDLERS) {}

    static RDMOps *instance;
  };

  const UID m_uid;
  bool m_identify_mode;
  std::auto_ptr<NetworkManagerInterface> m_network_manager;

  const RDMResponse *GetDeviceInfo(const RDMRequest *request);
  const RDMResponse *GetProductDetailList(const RDMRequest *request);
  const RDMResponse *GetIdentify(const RDMRequest *request);
  const RDMResponse *SetIdentify(const RDMRequest *request);
  const RDMResponse *GetManufacturerLabel(const RDMRequest *request);
  const RDMResponse *GetDeviceLabel(const RDMRequest *request);
  const RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
  const RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);
  const RDMResponse *GetListInterfaces(const RDMRequest *request);
  const RDMResponse *GetInterfaceLabel(const RDMRequest *request);
  const RDMResponse *GetInterfaceHardwareAddressType1(
      const RDMRequest *request);
  const RDMResponse *GetIPV4CurrentAddress(const RDMRequest *request);
  const RDMResponse *GetIPV4DefaultRoute(const RDMRequest *request);
  const RDMResponse *GetDNSHostname(const RDMRequest *request);
  const RDMResponse *GetDNSDomainName(const RDMRequest *request);
  const RDMResponse *GetDNSNameServer(const RDMRequest *request);

  static const ResponderOps<NetworkResponder>::ParamHandler PARAM_HANDLERS[];
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_NETWORKRESPONDER_H_
