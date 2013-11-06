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
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/NetworkResponder.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/rdm/ResponderNetworkController.h"

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::IPV4Address;
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
 * A class which represents a fake DNS getter.
 */
class FakeDNSGetter: public DNSGetter {
  public:
    FakeDNSGetter(const string &hostname,
                  const string &domain_name,
                  const NameServers &name_servers)
        : DNSGetter(),
          m_hostname(hostname),
          m_domain_name(domain_name),
          m_name_servers(name_servers) {
    }

  protected:
    string GetHostname();
    string GetDomainName();
    NameServers GetNameServers();

 private:
    string m_hostname;
    string m_domain_name;
    NameServers m_name_servers;
};


/**
 * Get a hostname
 */
string FakeDNSGetter::GetHostname() {
  return m_hostname;
}


/**
 * Get a domain name
 */
string FakeDNSGetter::GetDomainName() {
  return m_domain_name;
}

/**
 * Get name servers
 */
NameServers FakeDNSGetter::GetNameServers() {
  return m_name_servers;
}

/**
 * New NetworkResponder
 */
NetworkResponder::NetworkResponder(const UID &uid)
    : m_uid(uid),
      m_identify_mode(false) {
    vector<IPV4Address> name_servers;
    name_servers.push_back(IPV4Address::FromStringOrDie("10.0.0.1"));
    name_servers.push_back(IPV4Address::FromStringOrDie("10.0.0.2"));
    name_servers.push_back(IPV4Address::FromStringOrDie("10.0.0.3"));
    m_dns_getter = new FakeDNSGetter("foo", "bar.com", name_servers);
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

const RDMResponse *NetworkResponder::GetDNSHostname(
    const RDMRequest *request) {
  return ResponderHelper::GetDNSHostname(request, m_dns_getter);
}

const RDMResponse *NetworkResponder::GetDNSDomainName(
    const RDMRequest *request) {
  return ResponderHelper::GetDNSDomainName(request, m_dns_getter);
}

const RDMResponse *NetworkResponder::GetDNSNameServer(
    const RDMRequest *request) {
  return ResponderHelper::GetDNSNameServer(request, m_dns_getter);
}
}  // namespace rdm
}  // namespace ola
