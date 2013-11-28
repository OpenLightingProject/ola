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
 * NetworkResponder.cpp
 * Copyright (C) 2013 Peter Newman
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/stl/STLUtils.h"
#include "ola/network/MACAddress.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/NetworkResponder.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/RealGlobalNetworkGetter.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/rdm/ResponderNetworkController.h"
#include "common/network/FakeInterfacePicker.h"

namespace ola {
namespace rdm {

using ola::network::FakeInterfacePicker;
using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using ola::network::MACAddress;
using ola::network::NetworkToHost;
using std::string;
using std::vector;

NetworkResponder::RDMOps *NetworkResponder::RDMOps::instance = NULL;

const ResponderOps<NetworkResponder>::ParamHandler
    NetworkResponder::PARAM_HANDLERS[] = {
  { PID_DEVICE_INFO,
    &NetworkResponder::GetDeviceInfo,
    NULL},
  { PID_PRODUCT_DETAIL_ID_LIST,
    &NetworkResponder::GetProductDetailList,
    NULL},
  { PID_DEVICE_MODEL_DESCRIPTION,
    &NetworkResponder::GetDeviceModelDescription,
    NULL},
  { PID_MANUFACTURER_LABEL,
    &NetworkResponder::GetManufacturerLabel,
    NULL},
  { PID_DEVICE_LABEL,
    &NetworkResponder::GetDeviceLabel,
    NULL},
  { PID_SOFTWARE_VERSION_LABEL,
    &NetworkResponder::GetSoftwareVersionLabel,
    NULL},
  { PID_IDENTIFY_DEVICE,
    &NetworkResponder::GetIdentify,
    &NetworkResponder::SetIdentify},
  { PID_LIST_INTERFACES,
    &NetworkResponder::GetListInterfaces,
    NULL},
  { PID_INTERFACE_LABEL,
    &NetworkResponder::GetInterfaceLabel,
    NULL},
  { PID_INTERFACE_HARDWARE_ADDRESS_TYPE1,
    &NetworkResponder::GetInterfaceHardwareAddressType1,
    NULL},
  { PID_IPV4_CURRENT_ADDRESS,
    &NetworkResponder::GetIPV4CurrentAddress,
    NULL},
  { PID_IPV4_DEFAULT_ROUTE,
    &NetworkResponder::GetIPV4DefaultRoute,
    NULL},
  { PID_DNS_HOSTNAME,
    &NetworkResponder::GetDNSHostname,
    NULL},
  { PID_DNS_DOMAIN_NAME,
    &NetworkResponder::GetDNSDomainName,
    NULL},
  { PID_DNS_NAME_SERVER,
    &NetworkResponder::GetDNSNameServer,
    NULL},
  { 0, NULL, NULL},
};


/**
 * A class which represents a fake network getter.
 */
class FakeGlobalNetworkGetter: public GlobalNetworkGetter {
  public:
    FakeGlobalNetworkGetter(vector<Interface> &interfaces,
                            const IPV4Address ipv4_default_route,
                            const string &hostname,
                            const string &domain_name,
                            const vector<IPV4Address> &name_servers)
        : GlobalNetworkGetter(),
          m_ipv4_default_route(ipv4_default_route),
          m_hostname(hostname),
          m_domain_name(domain_name),
          m_name_servers(name_servers) {
      // Todo(Peter): should be FakeInterfacePicker when I can get it working
      //m_interface_picker.reset(InterfacePicker::NewPicker());
      m_interface_picker.reset(new FakeInterfacePicker(interfaces));
      if (interfaces.size() > 0) {}
    }

    const ola::network::InterfacePicker *GetInterfacePicker() const;
    bool GetDHCPStatus(const ola::network::Interface &iface) const;
    const IPV4Address GetIPV4DefaultRoute() const;
    const string GetHostname() const;
    const string GetDomainName() const;
    bool GetNameServers(vector<IPV4Address> *name_servers) const;

 private:
    auto_ptr<InterfacePicker> m_interface_picker;
    IPV4Address m_ipv4_default_route;
    string m_hostname;
    string m_domain_name;
    vector<IPV4Address> m_name_servers;
};


const InterfacePicker *FakeGlobalNetworkGetter::GetInterfacePicker() const {
  OLA_INFO << "Looking at get IF picker";
  (void) m_interface_picker.get()->GetInterfaces(false);
  OLA_INFO << "Got an IF picker";
  OLA_INFO << "Found " << m_interface_picker.get()->GetInterfaces(false).size() << " fake interfaces in IF picker fetch check";
  return m_interface_picker.get();
};


bool FakeGlobalNetworkGetter::GetDHCPStatus(
    const ola::network::Interface &iface) const {
  // TODO(Peter): Fixme - actually do the work!
  if (iface.index > 0) {}
  return false;
}


const IPV4Address FakeGlobalNetworkGetter::GetIPV4DefaultRoute() const {
  return m_ipv4_default_route;
}


const string FakeGlobalNetworkGetter::GetHostname() const {
  return m_hostname;
}


const string FakeGlobalNetworkGetter::GetDomainName() const {
  return m_domain_name;
}


bool FakeGlobalNetworkGetter::GetNameServers(
    vector<IPV4Address> *name_servers) const {
  *name_servers = m_name_servers;
  return true;
}


/**
 * New NetworkResponder
 */
NetworkResponder::NetworkResponder(const UID &uid)
    : m_uid(uid),
      m_identify_mode(false) {
   vector<Interface> interfaces;

   interfaces.push_back(Interface(
       "eth1",
       IPV4Address::FromStringOrDie("10.0.0.20"),
       IPV4Address::FromStringOrDie("10.0.0.255"),
       IPV4Address::FromStringOrDie("255.255.255.0"),
       MACAddress::FromStringOrDie("01:23:45:67:89:ab"),
       false,
       1));

   auto_ptr<InterfacePicker> test_picker;
	 test_picker.reset(new FakeInterfacePicker(interfaces));
	 OLA_INFO << "Found " << test_picker.get()->GetInterfaces(false).size() << " fake interfaces in test";

   vector<IPV4Address> name_servers;
   name_servers.push_back(IPV4Address::FromStringOrDie("10.0.0.1"));
   name_servers.push_back(IPV4Address::FromStringOrDie("10.0.0.2"));
   name_servers.push_back(IPV4Address::FromStringOrDie("10.0.0.3"));

   m_global_network_getter.reset(new FakeGlobalNetworkGetter(
       interfaces,
       IPV4Address::FromStringOrDie("10.0.0.254"),
       "foo",
       "bar.com",
       name_servers));
  //m_global_network_getter.reset(new RealGlobalNetworkGetter());
}


NetworkResponder::~NetworkResponder() {
}

/*
 * Handle an RDM Request
 */
void NetworkResponder::SendRDMRequest(const RDMRequest *request,
                                      RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ROOT_RDM_DEVICE, request,
                                       callback);
}

const RDMResponse *NetworkResponder::GetDeviceInfo(
    const RDMRequest *request) {
  return ResponderHelper::GetDeviceInfo(
      request, OLA_E137_2_MODEL, PRODUCT_CATEGORY_TEST,
      2, 0, 1, 1, ZERO_FOOTPRINT_DMX_ADDRESS, 0, 0);
}

const RDMResponse *NetworkResponder::GetProductDetailList(
    const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(
      request, vector<rdm_product_detail>(1, PRODUCT_DETAIL_TEST));
}

const RDMResponse *NetworkResponder::GetIdentify(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_mode);
}

const RDMResponse *NetworkResponder::SetIdentify(
    const RDMRequest *request) {
  bool old_value = m_identify_mode;
  const RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_mode);
  if (m_identify_mode != old_value) {
    OLA_INFO << "Network Device " << m_uid << ", identify mode "
             << (m_identify_mode ? "on" : "off");
  }
  return response;
}

const RDMResponse *NetworkResponder::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA Network Device");
}

const RDMResponse *NetworkResponder::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL);
}

const RDMResponse *NetworkResponder::GetDeviceLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Network Device");
}

const RDMResponse *NetworkResponder::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

const RDMResponse *NetworkResponder::GetListInterfaces(
    const RDMRequest *request) {
  return ResponderHelper::GetListInterfaces(request,
                                            m_global_network_getter.get());
}

const RDMResponse *NetworkResponder::GetInterfaceLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetInterfaceLabel(request,
                                            m_global_network_getter.get());
}

const RDMResponse *NetworkResponder::GetInterfaceHardwareAddressType1(
    const RDMRequest *request) {
  return ResponderHelper::GetInterfaceHardwareAddressType1(
      request,
      m_global_network_getter.get());
}

const RDMResponse *NetworkResponder::GetIPV4CurrentAddress(
    const RDMRequest *request) {
  return ResponderHelper::GetIPV4CurrentAddress(request,
                                                m_global_network_getter.get());
}

const RDMResponse *NetworkResponder::GetIPV4DefaultRoute(
    const RDMRequest *request) {
  return ResponderHelper::GetIPV4DefaultRoute(request,
                                              m_global_network_getter.get());
}

const RDMResponse *NetworkResponder::GetDNSHostname(
    const RDMRequest *request) {
  return ResponderHelper::GetDNSHostname(request,
                                         m_global_network_getter.get());
}

const RDMResponse *NetworkResponder::GetDNSDomainName(
    const RDMRequest *request) {
  return ResponderHelper::GetDNSDomainName(request,
                                           m_global_network_getter.get());
}

const RDMResponse *NetworkResponder::GetDNSNameServer(
    const RDMRequest *request) {
  return ResponderHelper::GetDNSNameServer(request,
                                           m_global_network_getter.get());
}
}  // namespace rdm
}  // namespace ola
