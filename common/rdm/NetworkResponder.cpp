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
 * NetworkResponder.cpp
 * Copyright (C) 2013 Peter Newman
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <iostream>
#include <string>
#include <vector>
#include "common/rdm/FakeNetworkManager.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/MACAddress.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/NetworkResponder.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/stl/STLUtils.h"

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::Interface;
using ola::network::InterfacePicker;
using ola::network::IPV4Address;
using ola::network::MACAddress;
using ola::network::NetworkToHost;
using std::string;
using std::vector;
using std::auto_ptr;

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
 * New NetworkResponder
 */
NetworkResponder::NetworkResponder(const UID &uid)
    : m_uid(uid),
      m_identify_mode(false) {
  vector<Interface> interfaces;

  interfaces.push_back(Interface(
      "eth0",
      IPV4Address::FromStringOrDie("10.0.0.20"),
      IPV4Address::FromStringOrDie("10.0.0.255"),
      IPV4Address::FromStringOrDie("255.255.0.0"),
      MACAddress::FromStringOrDie("01:23:45:67:89:ab"),
      false,
      1,
      Interface::ARP_ETHERNET_TYPE));

  interfaces.push_back(Interface(
      "eth2",
      IPV4Address::FromStringOrDie("192.168.0.1"),
      IPV4Address::FromStringOrDie("192.168.0.254"),
      IPV4Address::FromStringOrDie("255.255.255.0"),
      MACAddress::FromStringOrDie("45:67:89:ab:cd:ef"),
      false,
      2,
      Interface::ARP_ETHERNET_TYPE));

  vector<IPV4Address> name_servers;
  name_servers.push_back(IPV4Address::FromStringOrDie("10.0.0.1"));
  name_servers.push_back(IPV4Address::FromStringOrDie("10.0.0.2"));
  name_servers.push_back(IPV4Address::FromStringOrDie("10.0.0.3"));

  m_network_manager.reset(new FakeNetworkManager(
      interfaces,
      1,
      IPV4Address::FromStringOrDie("10.0.0.254"),
      "foo",
      "bar.com",
      name_servers));
}


NetworkResponder::~NetworkResponder() {
}

/*
 * Handle an RDM Request
 */
void NetworkResponder::SendRDMRequest(RDMRequest *request,
                                      RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ROOT_RDM_DEVICE, request,
                                       callback);
}

RDMResponse *NetworkResponder::GetDeviceInfo(
    const RDMRequest *request) {
  return ResponderHelper::GetDeviceInfo(
      request, OLA_E137_2_MODEL, PRODUCT_CATEGORY_TEST,
      2, 0, 1, 1, ZERO_FOOTPRINT_DMX_ADDRESS, 0, 0);
}

RDMResponse *NetworkResponder::GetProductDetailList(
    const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(
      request, vector<rdm_product_detail>(1, PRODUCT_DETAIL_TEST));
}

RDMResponse *NetworkResponder::GetIdentify(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_mode);
}

RDMResponse *NetworkResponder::SetIdentify(
    const RDMRequest *request) {
  bool old_value = m_identify_mode;
  RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_mode);
  if (m_identify_mode != old_value) {
    OLA_INFO << "Network Device " << m_uid << ", identify mode "
             << (m_identify_mode ? "on" : "off");
  }
  return response;
}

RDMResponse *NetworkResponder::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA Network Device");
}

RDMResponse *NetworkResponder::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL);
}

RDMResponse *NetworkResponder::GetDeviceLabel(const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Network Device");
}

RDMResponse *NetworkResponder::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

RDMResponse *NetworkResponder::GetListInterfaces(const RDMRequest *request) {
  return ResponderHelper::GetListInterfaces(request,
                                            m_network_manager.get());
}

RDMResponse *NetworkResponder::GetInterfaceLabel(const RDMRequest *request) {
  return ResponderHelper::GetInterfaceLabel(request,
                                            m_network_manager.get());
}

RDMResponse *NetworkResponder::GetInterfaceHardwareAddressType1(
    const RDMRequest *request) {
  return ResponderHelper::GetInterfaceHardwareAddressType1(
      request,
      m_network_manager.get());
}

RDMResponse *NetworkResponder::GetIPV4CurrentAddress(
    const RDMRequest *request) {
  return ResponderHelper::GetIPV4CurrentAddress(request,
                                                m_network_manager.get());
}

RDMResponse *NetworkResponder::GetIPV4DefaultRoute(const RDMRequest *request) {
  return ResponderHelper::GetIPV4DefaultRoute(request,
                                              m_network_manager.get());
}

RDMResponse *NetworkResponder::GetDNSHostname(const RDMRequest *request) {
  return ResponderHelper::GetDNSHostname(request,
                                         m_network_manager.get());
}

RDMResponse *NetworkResponder::GetDNSDomainName(const RDMRequest *request) {
  return ResponderHelper::GetDNSDomainName(request,
                                           m_network_manager.get());
}

RDMResponse *NetworkResponder::GetDNSNameServer(const RDMRequest *request) {
  return ResponderHelper::GetDNSNameServer(request,
                                           m_network_manager.get());
}
}  // namespace rdm
}  // namespace ola
