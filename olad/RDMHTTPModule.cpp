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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * RDMHTTPModule.cpp
 * This module acts as the HTTP -> olad gateway for RDM commands.
 * Copyright (C) 2010 Simon Newton
 */

#include <time.h>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ola/Callback.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/OlaCallbackClient.h"
#include "ola/StringUtils.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/RDMHelper.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/thread/Mutex.h"
#include "ola/web/Json.h"
#include "ola/web/JsonSections.h"
#include "olad/OlaServer.h"
#include "olad/OladHTTPServer.h"
#include "olad/RDMHTTPModule.h"

namespace ola {

using ola::OladHTTPServer;
using ola::client::OlaUniverse;
using ola::client::Result;
using ola::http::HTTPRequest;
using ola::http::HTTPResponse;
using ola::http::HTTPServer;
using ola::rdm::UID;
using ola::thread::MutexLocker;
using ola::web::BoolItem;
using ola::web::GenericItem;
using ola::web::HiddenItem;
using ola::web::JsonArray;
using ola::web::JsonObject;
using ola::web::JsonSection;
using ola::web::SelectItem;
using ola::web::StringItem;
using ola::web::UIntItem;
using std::endl;
using std::map;
using std::ostringstream;
using std::pair;
using std::set;
using std::string;
using std::vector;


const char RDMHTTPModule::BACKEND_DISCONNECTED_ERROR[] =
    "Failed to send request, client isn't connected";

// global URL params
const char RDMHTTPModule::HINT_KEY[] = "hint";
const char RDMHTTPModule::ID_KEY[] = "id";
const char RDMHTTPModule::SECTION_KEY[] = "section";
const char RDMHTTPModule::UID_KEY[] = "uid";

// URL params for particular sections
const char RDMHTTPModule::ADDRESS_FIELD[] = "address";
const char RDMHTTPModule::DIMMER_MINIMUM_DECREASING_FIELD[] = "min_increasing";
const char RDMHTTPModule::DIMMER_MINIMUM_INCREASING_FIELD[] = "min_decreasing";
const char RDMHTTPModule::DISPLAY_INVERT_FIELD[] = "invert";
const char RDMHTTPModule::GENERIC_BOOL_FIELD[] = "bool";
const char RDMHTTPModule::GENERIC_STRING_FIELD[] = "string";
const char RDMHTTPModule::GENERIC_UINT_FIELD[] = "int";
const char RDMHTTPModule::IDENTIFY_DEVICE_FIELD[] = "identify_device";
const char RDMHTTPModule::LABEL_FIELD[] = "label";
const char RDMHTTPModule::LANGUAGE_FIELD[] = "language";
const char RDMHTTPModule::RECORD_SENSOR_FIELD[] = "record";
const char RDMHTTPModule::SUB_DEVICE_FIELD[] = "sub_device";

// section identifiers
const char RDMHTTPModule::BOOT_SOFTWARE_SECTION[] = "boot_software";
const char RDMHTTPModule::CLOCK_SECTION[] = "clock";
const char RDMHTTPModule::COMMS_STATUS_SECTION[] = "comms_status";
const char RDMHTTPModule::CURVE_SECTION[] = "curve";
const char RDMHTTPModule::DEVICE_HOURS_SECTION[] = "device_hours";
const char RDMHTTPModule::DEVICE_INFO_SECTION[] = "device_info";
const char RDMHTTPModule::DEVICE_LABEL_SECTION[] = "device_label";
const char RDMHTTPModule::DIMMER_INFO_SECTION[] = "dimmer_info";
const char RDMHTTPModule::DIMMER_MAXIMUM_SECTION[] = "dimmer_maximum";
const char RDMHTTPModule::DIMMER_MINIMUM_SECTION[] = "dimmer_minimum";
const char RDMHTTPModule::DISPLAY_INVERT_SECTION[] = "display_invert";
const char RDMHTTPModule::DISPLAY_LEVEL_SECTION[] = "display_level";
const char RDMHTTPModule::DMX_ADDRESS_SECTION[] = "dmx_address";
const char RDMHTTPModule::DNS_DOMAIN_NAME_SECTION[] = "dns_domain_name";
const char RDMHTTPModule::DNS_HOSTNAME_SECTION[] = "dns_hostname";
const char RDMHTTPModule::FACTORY_DEFAULTS_SECTION[] = "factory_defaults";
const char RDMHTTPModule::IDENTIFY_DEVICE_SECTION[] = "identify_device";
const char RDMHTTPModule::LAMP_HOURS_SECTION[] = "lamp_hours";
const char RDMHTTPModule::LAMP_MODE_SECTION[] = "lamp_mode";
const char RDMHTTPModule::LAMP_STATE_SECTION[] = "lamp_state";
const char RDMHTTPModule::LAMP_STRIKES_SECTION[] = "lamp_strikes";
const char RDMHTTPModule::LANGUAGE_SECTION[] = "language";
const char RDMHTTPModule::MANUFACTURER_LABEL_SECTION[] = "manufacturer_label";
const char RDMHTTPModule::PAN_INVERT_SECTION[] = "pan_invert";
const char RDMHTTPModule::PAN_TILT_SWAP_SECTION[] = "pan_tilt_swap";
const char RDMHTTPModule::PERSONALITY_SECTION[] = "personality";
const char RDMHTTPModule::POWER_CYCLES_SECTION[] = "power_cycles";
const char RDMHTTPModule::POWER_STATE_SECTION[] = "power_state";
const char RDMHTTPModule::PRODUCT_DETAIL_SECTION[] = "product_detail";
const char RDMHTTPModule::PROXIED_DEVICES_SECTION[] = "proxied_devices";
const char RDMHTTPModule::RESET_DEVICE_SECTION[] = "reset_device";
const char RDMHTTPModule::SENSOR_SECTION[] = "sensor";
const char RDMHTTPModule::TILT_INVERT_SECTION[] = "tilt_invert";

// section names
const char RDMHTTPModule::BOOT_SOFTWARE_SECTION_NAME[] =
  "Boot Software Version";
const char RDMHTTPModule::CLOCK_SECTION_NAME[] = "Clock";
const char RDMHTTPModule::COMMS_STATUS_SECTION_NAME[] = "Communication Status";
const char RDMHTTPModule::CURVE_SECTION_NAME[] = "Dimmer Curve";
const char RDMHTTPModule::DEVICE_HOURS_SECTION_NAME[] = "Device Hours";
const char RDMHTTPModule::DEVICE_INFO_SECTION_NAME[] = "Device Info";
const char RDMHTTPModule::DEVICE_LABEL_SECTION_NAME[] = "Device Label";
const char RDMHTTPModule::DIMMER_INFO_SECTION_NAME[] = "Dimmer Info";
const char RDMHTTPModule::DIMMER_MAXIMUM_SECTION_NAME[] = "Dimmer Maximum";
const char RDMHTTPModule::DIMMER_MINIMUM_SECTION_NAME[] = "Dimmer Minimum";
const char RDMHTTPModule::DISPLAY_INVERT_SECTION_NAME[] = "Display Invert";
const char RDMHTTPModule::DISPLAY_LEVEL_SECTION_NAME[] = "Display Level";
const char RDMHTTPModule::DMX_ADDRESS_SECTION_NAME[] = "DMX Start Address";
const char RDMHTTPModule::DNS_DOMAIN_NAME_SECTION_NAME[] = "DNS Domain Name";
const char RDMHTTPModule::DNS_HOSTNAME_SECTION_NAME[] = "DNS Hostname";
const char RDMHTTPModule::FACTORY_DEFAULTS_SECTION_NAME[] = "Factory Defaults";
const char RDMHTTPModule::IDENTIFY_DEVICE_SECTION_NAME[] = "Identify Device";
const char RDMHTTPModule::LAMP_HOURS_SECTION_NAME[] = "Lamp Hours";
const char RDMHTTPModule::LAMP_MODE_SECTION_NAME[] = "Lamp On Mode";
const char RDMHTTPModule::LAMP_STATE_SECTION_NAME[] = "Lamp State";
const char RDMHTTPModule::LAMP_STRIKES_SECTION_NAME[] = "Lamp Strikes";
const char RDMHTTPModule::LANGUAGE_SECTION_NAME[] = "Language";
const char RDMHTTPModule::MANUFACTURER_LABEL_SECTION_NAME[] =
  "Manufacturer Label";
const char RDMHTTPModule::PAN_INVERT_SECTION_NAME[] = "Pan Invert";
const char RDMHTTPModule::PAN_TILT_SWAP_SECTION_NAME[] = "Pan/Tilt Swap";
const char RDMHTTPModule::PERSONALITY_SECTION_NAME[] = "DMX Personality";
const char RDMHTTPModule::POWER_CYCLES_SECTION_NAME[] = "Device Power Cycles";
const char RDMHTTPModule::POWER_STATE_SECTION_NAME[] = "Power State";
const char RDMHTTPModule::PRODUCT_DETAIL_SECTION_NAME[] = "Product Details";
const char RDMHTTPModule::PROXIED_DEVICES_SECTION_NAME[] = "Proxied Devices";
const char RDMHTTPModule::RESET_DEVICE_SECTION_NAME[] = "Reset Device";
const char RDMHTTPModule::TILT_INVERT_SECTION_NAME[] = "Tilt Invert";

RDMHTTPModule::RDMHTTPModule(HTTPServer *http_server,
                             client::OlaClient *client)
    : m_server(http_server),
      m_client(client),
      m_shim(client),
      m_rdm_api(&m_shim),
      m_pid_store(NULL) {

  m_server->RegisterHandler(
      "/rdm/run_discovery",
      NewCallback(this, &RDMHTTPModule::RunRDMDiscovery));
  m_server->RegisterHandler(
      "/json/rdm/uids",
      NewCallback(this, &RDMHTTPModule::JsonUIDs));
  m_server->RegisterHandler(
      "/json/rdm/uid_info",
      NewCallback(this, &RDMHTTPModule::JsonUIDInfo));
  // Deprecated for clarity, use uid_identify_device instead
  m_server->RegisterHandler(
      "/json/rdm/uid_identify",
      NewCallback(this, &RDMHTTPModule::JsonUIDIdentifyDevice));
  m_server->RegisterHandler(
      "/json/rdm/uid_identify_device",
      NewCallback(this, &RDMHTTPModule::JsonUIDIdentifyDevice));
  m_server->RegisterHandler(
      "/json/rdm/uid_personalities",
      NewCallback(this, &RDMHTTPModule::JsonUIDPersonalities));
  m_server->RegisterHandler(
      "/json/rdm/supported_pids",
      NewCallback(this, &RDMHTTPModule::JsonSupportedPIDs));
  m_server->RegisterHandler(
      "/json/rdm/supported_sections",
      NewCallback(this, &RDMHTTPModule::JsonSupportedSections));
  m_server->RegisterHandler(
      "/json/rdm/section_info",
      NewCallback(this, &RDMHTTPModule::JsonSectionInfo));
  m_server->RegisterHandler(
      "/json/rdm/set_section_info",
      NewCallback(this, &RDMHTTPModule::JsonSaveSectionInfo));
}


/*
 * Teardown
 */
RDMHTTPModule::~RDMHTTPModule() {
  map<unsigned int, uid_resolution_state*>::iterator uid_iter;
  for (uid_iter = m_universe_uids.begin(); uid_iter != m_universe_uids.end();
       uid_iter++) {
    delete uid_iter->second;
  }
  m_universe_uids.clear();
}

/**
 * @brief Can be called while the server is running. Ownership is not transferred.
 */
void RDMHTTPModule::SetPidStore(const ola::rdm::RootPidStore *pid_store) {
  MutexLocker lock(&m_pid_store_mu);
  m_pid_store = pid_store;
}


/**
 * @brief Run RDM discovery for a universe
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int RDMHTTPModule::RunRDMDiscovery(const HTTPRequest *request,
                                   HTTPResponse *response) {
  if (request->CheckParameterExists(OladHTTPServer::HELP_PARAMETER)) {
    return OladHTTPServer::ServeUsage(response,
                                      "?id=[universe]&amp;incremental=true");
  }
  unsigned int universe_id;
  if (!CheckForInvalidId(request, &universe_id)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  string incremental_str = request->GetParameter("incremental");
  bool incremental = incremental_str == "true";

  m_client->RunDiscovery(
      universe_id,
      incremental ? client::DISCOVERY_INCREMENTAL : client::DISCOVERY_FULL,
      NewSingleCallback(this, &RDMHTTPModule::HandleUIDList,
                        response, universe_id));

  return MHD_YES;
}


/**
 * @brief Return the list of uids for this universe as json
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int RDMHTTPModule::JsonUIDs(const HTTPRequest *request,
                            HTTPResponse *response) {
  if (request->CheckParameterExists(OladHTTPServer::HELP_PARAMETER)) {
    return OladHTTPServer::ServeUsage(response, "?id=[universe]");
  }
  unsigned int universe_id;
  if (!CheckForInvalidId(request, &universe_id)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  m_client->RunDiscovery(
      universe_id,
      client::DISCOVERY_CACHED,
      NewSingleCallback(this,
                        &RDMHTTPModule::HandleUIDList,
                        response,
                        universe_id));
  return MHD_YES;
}


/**
 * @brief Return the device info for this uid.
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int RDMHTTPModule::JsonUIDInfo(const HTTPRequest *request,
                               HTTPResponse *response) {
  if (request->CheckParameterExists(OladHTTPServer::HELP_PARAMETER)) {
    return OladHTTPServer::ServeUsage(response, "?id=[universe]&amp;uid=[uid]");
  }
  unsigned int universe_id;
  if (!CheckForInvalidId(request, &universe_id)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  UID *uid = NULL;
  if (!CheckForInvalidUid(request, &uid)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  string error;
  bool ok = m_rdm_api.GetDeviceInfo(
      universe_id,
      *uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::UIDInfoHandler,
                        response),
      &error);
  delete uid;

  if (!ok) {
    return m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR);
  }
  return MHD_YES;
}


/**
 * @brief Returns the identify state for the device.
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int RDMHTTPModule::JsonUIDIdentifyDevice(const HTTPRequest *request,
                                         HTTPResponse *response) {
  if (request->CheckParameterExists(OladHTTPServer::HELP_PARAMETER)) {
    return OladHTTPServer::ServeUsage(response, "?id=[universe]&amp;uid=[uid]");
  }
  unsigned int universe_id;
  if (!CheckForInvalidId(request, &universe_id)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  UID *uid = NULL;
  if (!CheckForInvalidUid(request, &uid)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  string error;
  bool ok = m_rdm_api.GetIdentifyDevice(
      universe_id,
      *uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::UIDIdentifyDeviceHandler,
                        response),
      &error);
  delete uid;

  if (!ok) {
    return m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR);
  }
  return MHD_YES;
}


/**
 * @brief Returns the personalities on the device
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int RDMHTTPModule::JsonUIDPersonalities(const HTTPRequest *request,
                                        HTTPResponse *response) {
  if (request->CheckParameterExists(OladHTTPServer::HELP_PARAMETER)) {
    return OladHTTPServer::ServeUsage(response, "?id=[universe]&amp;uid=[uid]");
  }
  unsigned int universe_id;
  if (!CheckForInvalidId(request, &universe_id)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  UID *uid = NULL;
  if (!CheckForInvalidUid(request, &uid)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  string error = GetPersonalities(request, response, universe_id, *uid, false,
                                  true);

  delete uid;
  if (!error.empty()) {
    return m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR + error);
  }
  return MHD_YES;
}


/**
 * @brief Return a list of PIDs supported by this device.
 *
 * This isn't used by the UI but it's useful for debugging.
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 * @sa JsonSupportedSections
 */
int RDMHTTPModule::JsonSupportedPIDs(const HTTPRequest *request,
                                     HTTPResponse *response) {
  if (request->CheckParameterExists(OladHTTPServer::HELP_PARAMETER)) {
    return OladHTTPServer::ServeUsage(response, "?id=[universe]&amp;uid=[uid]");
  }
  unsigned int universe_id;
  if (!CheckForInvalidId(request, &universe_id)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  UID *uid = NULL;
  if (!CheckForInvalidUid(request, &uid)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  string error;
  bool ok = m_rdm_api.GetSupportedParameters(
      universe_id,
      *uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::SupportedParamsHandler,
                        response),
      &error);
  delete uid;

  if (!ok) {
    return m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR);
  }
  return MHD_YES;
}


/**
 * @brief Return a list of sections to display in the RDM control panel.
 *
 * We use the response from SUPPORTED_PARAMS and DEVICE_INFO to decide which
 * PIDs exist.
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int RDMHTTPModule::JsonSupportedSections(const HTTPRequest *request,
                                         HTTPResponse *response) {
  if (request->CheckParameterExists(OladHTTPServer::HELP_PARAMETER)) {
    return OladHTTPServer::ServeUsage(response, "?id=[universe]&amp;uid=[uid]");
  }

  unsigned int universe_id;
  if (!CheckForInvalidId(request, &universe_id)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  UID *uid = NULL;
  if (!CheckForInvalidUid(request, &uid)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  string error;
  bool ok = m_rdm_api.GetSupportedParameters(
      universe_id,
      *uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::SupportedSectionsHandler,
                        response,
                        universe_id,
                        *uid),
      &error);
  delete uid;

  if (!ok) {
    return m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR);
  }
  return MHD_YES;
}


/**
 * @brief Get the information required to render a section in the RDM
 *   controller panel
 */
int RDMHTTPModule::JsonSectionInfo(const HTTPRequest *request,
                                   HTTPResponse *response) {
  if (request->CheckParameterExists(OladHTTPServer::HELP_PARAMETER)) {
    return OladHTTPServer::ServeUsage(response, "?id=[universe]&amp;uid=[uid]"
                                      "&amp;section=[section]<br />See "
                                      "/json/rdm/supported_sections for "
                                      "sections");
  }
  unsigned int universe_id;
  if (!CheckForInvalidId(request, &universe_id)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  UID *uid = NULL;
  if (!CheckForInvalidUid(request, &uid)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  string section_id = request->GetParameter(SECTION_KEY);
  string error;
  if (section_id == PROXIED_DEVICES_SECTION) {
    error = GetProxiedDevices(response, universe_id, *uid);
  } else if (section_id == COMMS_STATUS_SECTION) {
    error = GetCommStatus(response, universe_id, *uid);
  } else if (section_id == DEVICE_INFO_SECTION) {
    error = GetDeviceInfo(request, response, universe_id, *uid);
  } else if (section_id == PRODUCT_DETAIL_SECTION) {
    error = GetProductIds(request, response, universe_id, *uid);
  } else if (section_id == MANUFACTURER_LABEL_SECTION) {
    error = GetManufacturerLabel(request, response, universe_id, *uid);
  } else if (section_id == DEVICE_LABEL_SECTION) {
    error = GetDeviceLabel(request, response, universe_id, *uid);
  } else if (section_id == FACTORY_DEFAULTS_SECTION) {
    error = GetFactoryDefaults(response, universe_id, *uid);
  } else if (section_id == LANGUAGE_SECTION) {
    error = GetLanguage(response, universe_id, *uid);
  } else if (section_id == BOOT_SOFTWARE_SECTION) {
    error = GetBootSoftware(response, universe_id, *uid);
  } else if (section_id == PERSONALITY_SECTION) {
    error = GetPersonalities(request, response, universe_id, *uid, true);
  } else if (section_id == DMX_ADDRESS_SECTION) {
    error = GetStartAddress(request, response, universe_id, *uid);
  } else if (section_id == SENSOR_SECTION) {
    error = GetSensor(request, response, universe_id, *uid);
  } else if (section_id == DEVICE_HOURS_SECTION) {
    error = GetDeviceHours(request, response, universe_id, *uid);
  } else if (section_id == LAMP_HOURS_SECTION) {
    error = GetLampHours(request, response, universe_id, *uid);
  } else if (section_id == LAMP_MODE_SECTION) {
    error = GetLampMode(request, response, universe_id, *uid);
  } else if (section_id == LAMP_STATE_SECTION) {
    error = GetLampState(request, response, universe_id, *uid);
  } else if (section_id == LAMP_STRIKES_SECTION) {
    error = GetLampStrikes(request, response, universe_id, *uid);
  } else if (section_id == POWER_CYCLES_SECTION) {
    error = GetPowerCycles(request, response, universe_id, *uid);
  } else if (section_id == DISPLAY_INVERT_SECTION) {
    error = GetDisplayInvert(response, universe_id, *uid);
  } else if (section_id == DISPLAY_LEVEL_SECTION) {
    error = GetDisplayLevel(response, universe_id, *uid);
  } else if (section_id == PAN_INVERT_SECTION) {
    error = GetPanInvert(response, universe_id, *uid);
  } else if (section_id == TILT_INVERT_SECTION) {
    error = GetTiltInvert(response, universe_id, *uid);
  } else if (section_id == PAN_TILT_SWAP_SECTION) {
    error = GetPanTiltSwap(response, universe_id, *uid);
  } else if (section_id == CLOCK_SECTION) {
    error = GetClock(response, universe_id, *uid);
  } else if (section_id == IDENTIFY_DEVICE_SECTION) {
    error = GetIdentifyDevice(response, universe_id, *uid);
  } else if (section_id == POWER_STATE_SECTION) {
    error = GetPowerState(response, universe_id, *uid);
  } else if (section_id == RESET_DEVICE_SECTION) {
    // No get command available, so just generate the JSON
    error = GetResetDevice(response);
  } else if (section_id == DNS_HOSTNAME_SECTION) {
    error = GetDnsHostname(response, universe_id, *uid);
  } else if (section_id == DNS_DOMAIN_NAME_SECTION) {
    error = GetDnsDomainName(response, universe_id, *uid);
  } else if (section_id == CURVE_SECTION) {
    error = GetCurve(request, response, universe_id, *uid, true);
  } else if (section_id == DIMMER_INFO_SECTION) {
    error = GetDimmerInfo(response, universe_id, *uid);
  } else if (section_id == DIMMER_MINIMUM_SECTION) {
    error = GetDimmerMinimumLevels(response, universe_id, *uid);
  } else if (section_id == DIMMER_MAXIMUM_SECTION) {
    error = GetDimmerMaximumLevel(response, universe_id, *uid);
  } else {
    OLA_INFO << "Missing or unknown section id: " << section_id;
    delete uid;
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  delete uid;
  if (!error.empty()) {
    return m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR + error);
  }
  return MHD_YES;
}


/**
 * Save the information for a section or item.
 */
int RDMHTTPModule::JsonSaveSectionInfo(const HTTPRequest *request,
                                       HTTPResponse *response) {
  if (request->CheckParameterExists(OladHTTPServer::HELP_PARAMETER)) {
    return OladHTTPServer::ServeUsage(response, "?id=[universe]&amp;uid=[uid]"
                                      "&amp;section=[section]<br />See "
                                      "/json/rdm/supported_sections for "
                                      "sections");
  }
  unsigned int universe_id;
  if (!CheckForInvalidId(request, &universe_id)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  UID *uid = NULL;
  if (!CheckForInvalidUid(request, &uid)) {
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  string section_id = request->GetParameter(SECTION_KEY);
  string error;
  if (section_id == DEVICE_LABEL_SECTION) {
    error = SetDeviceLabel(request, response, universe_id, *uid);
  } else if (section_id == COMMS_STATUS_SECTION) {
    error = ClearCommsCounters(response, universe_id, *uid);
  } else if (section_id == FACTORY_DEFAULTS_SECTION) {
    error = SetFactoryDefault(response, universe_id, *uid);
  } else if (section_id == LANGUAGE_SECTION) {
    error = SetLanguage(request, response, universe_id, *uid);
  } else if (section_id == PERSONALITY_SECTION) {
    error = SetPersonality(request, response, universe_id, *uid);
  } else if (section_id == DMX_ADDRESS_SECTION) {
    error = SetStartAddress(request, response, universe_id, *uid);
  } else if (section_id == SENSOR_SECTION) {
    error = RecordSensor(request, response, universe_id, *uid);
  } else if (section_id == DEVICE_HOURS_SECTION) {
    error = SetDeviceHours(request, response, universe_id, *uid);
  } else if (section_id == LAMP_HOURS_SECTION) {
    error = SetLampHours(request, response, universe_id, *uid);
  } else if (section_id == LAMP_MODE_SECTION) {
    error = SetLampMode(request, response, universe_id, *uid);
  } else if (section_id == LAMP_STATE_SECTION) {
    error = SetLampState(request, response, universe_id, *uid);
  } else if (section_id == LAMP_STRIKES_SECTION) {
    error = SetLampStrikes(request, response, universe_id, *uid);
  } else if (section_id == POWER_CYCLES_SECTION) {
    error = SetPowerCycles(request, response, universe_id, *uid);
  } else if (section_id == DISPLAY_INVERT_SECTION) {
    error = SetDisplayInvert(request, response, universe_id, *uid);
  } else if (section_id == DISPLAY_LEVEL_SECTION) {
    error = SetDisplayLevel(request, response, universe_id, *uid);
  } else if (section_id == PAN_INVERT_SECTION) {
    error = SetPanInvert(request, response, universe_id, *uid);
  } else if (section_id == TILT_INVERT_SECTION) {
    error = SetTiltInvert(request, response, universe_id, *uid);
  } else if (section_id == PAN_TILT_SWAP_SECTION) {
    error = SetPanTiltSwap(request, response, universe_id, *uid);
  } else if (section_id == CLOCK_SECTION) {
    error = SyncClock(response, universe_id, *uid);
  } else if (section_id == IDENTIFY_DEVICE_SECTION) {
    error = SetIdentifyDevice(request, response, universe_id, *uid);
  } else if (section_id == POWER_STATE_SECTION) {
    error = SetPowerState(request, response, universe_id, *uid);
  } else if (section_id == RESET_DEVICE_SECTION) {
    error = SetResetDevice(request, response, universe_id, *uid);
  } else if (section_id == DNS_HOSTNAME_SECTION) {
    error = SetDnsHostname(request, response, universe_id, *uid);
  } else if (section_id == DNS_DOMAIN_NAME_SECTION) {
    error = SetDnsDomainName(request, response, universe_id, *uid);
  } else if (section_id == CURVE_SECTION) {
    error = SetCurve(request, response, universe_id, *uid);
  } else if (section_id == DIMMER_MINIMUM_SECTION) {
    error = SetDimmerMinimumLevels(request, response, universe_id, *uid);
  } else if (section_id == DIMMER_MAXIMUM_SECTION) {
    error = SetDimmerMaximumLevel(request, response, universe_id, *uid);
  } else {
    OLA_INFO << "Missing or unknown section id: " << section_id;
    delete uid;
    return OladHTTPServer::ServeHelpRedirect(response);
  }

  delete uid;
  if (!error.empty()) {
    return RespondWithError(response, error);
  }
  return MHD_YES;
}


/**
 * This is called from the main http server whenever a new list of active
 * universes is received. It's used to prune the uid map so we don't bother
 * trying to resolve uids for universes that no longer exist.
 */
void RDMHTTPModule::PruneUniverseList(const vector<OlaUniverse> &universes) {
  map<unsigned int, uid_resolution_state*>::iterator uid_iter;
  for (uid_iter = m_universe_uids.begin(); uid_iter != m_universe_uids.end();
       uid_iter++) {
    uid_iter->second->active = false;
  }

  vector<OlaUniverse>::const_iterator iter;
  for (iter = universes.begin(); iter != universes.end(); ++iter) {
    uid_iter = m_universe_uids.find(iter->Id());
    if (uid_iter != m_universe_uids.end()) {
      uid_iter->second->active = true;
    }
  }

  // clean up the uid map for those universes that no longer exist
  for (uid_iter = m_universe_uids.begin(); uid_iter != m_universe_uids.end();) {
    if (!uid_iter->second->active) {
      OLA_DEBUG << "removing " << uid_iter->first << " from the uid map";
      delete uid_iter->second;
      m_universe_uids.erase(uid_iter++);
    } else {
      uid_iter++;
    }
  }
}


/*
 * @brief Handle the UID list response.
 * @param response the HTTPResponse that is associated with the request.
 * @param uids the UIDs for this response.
 * @param error an error string.
 */
void RDMHTTPModule::HandleUIDList(HTTPResponse *response,
                                  unsigned int universe_id,
                                  const Result &result,
                                  const ola::rdm::UIDSet &uids) {
  if (!result.Success()) {
    m_server->ServeError(response, result.Error());
    return;
  }
  ola::rdm::UIDSet::Iterator iter = uids.Begin();
  uid_resolution_state *uid_state = GetUniverseUidsOrCreate(universe_id);

  // mark all uids as inactive so we can remove the unused ones at the end
  map<UID, resolved_uid>::iterator uid_iter;
  for (uid_iter = uid_state->resolved_uids.begin();
       uid_iter != uid_state->resolved_uids.end(); ++uid_iter)
    uid_iter->second.active = false;

  JsonObject json;
  json.Add("universe", universe_id);
  JsonArray *json_uids = json.AddArray("uids");

  for (; iter != uids.End(); ++iter) {
    uid_iter = uid_state->resolved_uids.find(*iter);

    string manufacturer = "";
    string device = "";

    if (uid_iter == uid_state->resolved_uids.end()) {
      // schedule resolution
      uid_state->pending_uids.push(std::make_pair(*iter, RESOLVE_MANUFACTURER));
      uid_state->pending_uids.push(std::make_pair(*iter, RESOLVE_DEVICE));
      resolved_uid uid_descriptor = {"", "", true};
      uid_state->resolved_uids[*iter] = uid_descriptor;
      OLA_INFO << "Adding UID " << *iter << " to resolution queue";
    } else {
      manufacturer = uid_iter->second.manufacturer;
      device = uid_iter->second.device;
      uid_iter->second.active = true;
    }

    JsonObject *json_uid = json_uids->AppendObject();
    json_uid->Add("manufacturer_id", iter->ManufacturerId());
    json_uid->Add("device_id", iter->DeviceId());
    json_uid->Add("device", device);
    json_uid->Add("manufacturer", manufacturer);
    json_uid->Add("uid", iter->ToString());
  }

  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(json);
  delete response;

  // remove any old UIDs
  for (uid_iter = uid_state->resolved_uids.begin();
       uid_iter != uid_state->resolved_uids.end();) {
    if (!uid_iter->second.active) {
      OLA_INFO << "Removed UID " << uid_iter->first;
      uid_state->resolved_uids.erase(uid_iter++);
    } else {
      ++uid_iter;
    }
  }

  if (!uid_state->uid_resolution_running) {
    ResolveNextUID(universe_id);
  }
}


/*
 * @brief Send the RDM command needed to resolve the next uid in the queue
 * @param universe_id the universe id to resolve the next UID for.
 */
void RDMHTTPModule::ResolveNextUID(unsigned int universe_id) {
  bool sent_request = false;
  string error;
  uid_resolution_state *uid_state = GetUniverseUids(universe_id);

  if (!uid_state) {
    return;
  }

  while (!sent_request) {
    if (uid_state->pending_uids.empty()) {
      uid_state->uid_resolution_running = false;
      return;
    }
    uid_state->uid_resolution_running = true;

    pair<UID, uid_resolve_action> &uid_action_pair =
      uid_state->pending_uids.front();
    if (uid_action_pair.second == RESOLVE_MANUFACTURER) {
      OLA_INFO << "sending manufacturer request for " << uid_action_pair.first;
      sent_request = m_rdm_api.GetManufacturerLabel(
          universe_id,
          uid_action_pair.first,
          ola::rdm::ROOT_RDM_DEVICE,
          NewSingleCallback(this,
                            &RDMHTTPModule::UpdateUIDManufacturerLabel,
                            universe_id,
                            uid_action_pair.first),
          &error);
      uid_state->pending_uids.pop();
    } else if (uid_action_pair.second == RESOLVE_DEVICE) {
      OLA_INFO << "sending device request for " << uid_action_pair.first;
      sent_request = m_rdm_api.GetDeviceLabel(
          universe_id,
          uid_action_pair.first,
          ola::rdm::ROOT_RDM_DEVICE,
          NewSingleCallback(this,
                            &RDMHTTPModule::UpdateUIDDeviceLabel,
                            universe_id,
                            uid_action_pair.first),
          &error);
      uid_state->pending_uids.pop();
    } else {
      OLA_WARN << "Unknown UID resolve action " <<
        static_cast<int>(uid_action_pair.second);
    }
  }
}

/*
 * @brief Handle the manufacturer label response.
 */
void RDMHTTPModule::UpdateUIDManufacturerLabel(
    unsigned int universe,
    UID uid,
    const ola::rdm::ResponseStatus &status,
    const string &manufacturer_label) {
  uid_resolution_state *uid_state = GetUniverseUids(universe);

  if (!uid_state) {
    return;
  }

  if (CheckForRDMSuccess(status)) {
    map<UID, resolved_uid>::iterator uid_iter;
    uid_iter = uid_state->resolved_uids.find(uid);
    if (uid_iter != uid_state->resolved_uids.end()) {
      uid_iter->second.manufacturer = manufacturer_label;
    }
  }
  ResolveNextUID(universe);
}


/*
 * @brief Handle the device label response.
 */
void RDMHTTPModule::UpdateUIDDeviceLabel(
    unsigned int universe,
    UID uid,
    const ola::rdm::ResponseStatus &status,
    const string &device_label) {
  uid_resolution_state *uid_state = GetUniverseUids(universe);

  if (!uid_state) {
    return;
  }

  if (CheckForRDMSuccess(status)) {
    map<UID, resolved_uid>::iterator uid_iter;
    uid_iter = uid_state->resolved_uids.find(uid);
    if (uid_iter != uid_state->resolved_uids.end()) {
      uid_iter->second.device = device_label;
    }
  }
  ResolveNextUID(universe);
}


/*
 * @brief Get the UID resolution state for a particular universe
 * @param universe the id of the universe to get the state for
 */
RDMHTTPModule::uid_resolution_state *RDMHTTPModule::GetUniverseUids(
    unsigned int universe) {
  map<unsigned int, uid_resolution_state*>::iterator iter =
    m_universe_uids.find(universe);
  return iter == m_universe_uids.end() ? NULL : iter->second;
}


/*
 * @brief Get the UID resolution state for a particular universe or create one if it
 * doesn't exist.
 * @param universe the id of the universe to get the state for
 */
RDMHTTPModule::uid_resolution_state *RDMHTTPModule::GetUniverseUidsOrCreate(
    unsigned int universe) {
  map<unsigned int, uid_resolution_state*>::iterator iter =
    m_universe_uids.find(universe);

  if (iter == m_universe_uids.end()) {
    OLA_DEBUG << "Adding a new state entry for " << universe;
    uid_resolution_state *state  = new uid_resolution_state();
    state->uid_resolution_running = false;
    state->active = true;
    pair<unsigned int, uid_resolution_state*> p(universe, state);
    iter = m_universe_uids.insert(p).first;
  }
  return iter->second;
}


/**
 * @brief Handle the Device Info response and build the JSON
 */
void RDMHTTPModule::UIDInfoHandler(HTTPResponse *response,
                                   const ola::rdm::ResponseStatus &status,
                                   const ola::rdm::DeviceDescriptor &device) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonObject json;
  json.Add("error", "");
  json.Add("address", device.dmx_start_address);
  json.Add("footprint", device.dmx_footprint);
  json.Add("personality", static_cast<int>(device.current_personality));
  json.Add("personality_count", static_cast<int>(device.personality_count));

  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(json);
  delete response;
}


/**
 * @brief Handle the identify device response and build the JSON
 */
void RDMHTTPModule::UIDIdentifyDeviceHandler(
    HTTPResponse *response,
    const ola::rdm::ResponseStatus &status,
    bool value) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonObject json;
  json.Add("error", "");
  json.Add("identify_device", value);

  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(json);
  delete response;
}


/**
 * @brief Send the response to a DMX personality section
 */
void RDMHTTPModule::SendPersonalityResponse(HTTPResponse *response,
                                            personality_info *info) {
  JsonObject json;
  json.Add("error", "");
  JsonArray *personalities = json.AddArray("personalities");

  unsigned int i = 1;
  while (i <= info->total && i <= info->personalities.size()) {
    if (info->personalities[i - 1].first != INVALID_PERSONALITY) {
      JsonObject *personality = personalities->AppendObject();
      personality->Add("name", info->personalities[i - 1].second);
      personality->Add("index", i);
      personality->Add("footprint", info->personalities[i - 1].first);
    }
    i++;
  }
  json.Add("selected", info->active);

  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(json);
  delete info->uid;
  delete info;
}


/*
 * @brief Handle the response from a supported params request
 */
void RDMHTTPModule::SupportedParamsHandler(
    HTTPResponse *response,
    const ola::rdm::ResponseStatus &status,
    const vector<uint16_t> &pids) {
  JsonObject json;
  if (CheckForRDMSuccess(status)) {
    JsonArray *pids_json = json.AddArray("pids");
    vector<uint16_t>::const_iterator iter = pids.begin();
    for (; iter != pids.end(); ++iter)
      pids_json->Append(*iter);
  }

  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(json);
  delete response;
}


/**
 * @brief Takes the supported PIDs for a device and come up with the list of
 * sections to display in the RDM panel
 */
void RDMHTTPModule::SupportedSectionsHandler(
    HTTPResponse *response,
    unsigned int universe_id,
    UID uid,
    const ola::rdm::ResponseStatus &status,
    const vector<uint16_t> &pid_list) {
  string error;

  // nacks here are ok if the device doesn't support SUPPORTED_PARAMS
  if (!CheckForRDMSuccess(status) && !status.WasNacked()) {
    m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR + error);
    return;
  }

  m_rdm_api.GetDeviceInfo(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::SupportedSectionsDeviceInfoHandler,
                        response,
                        pid_list),
      &error);
  if (!error.empty())
    m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR + error);
}


/**
 * @brief Handle the second part of the supported sections request.
 */
void RDMHTTPModule::SupportedSectionsDeviceInfoHandler(
    HTTPResponse *response,
    const vector<uint16_t> pid_list,
    const ola::rdm::ResponseStatus &status,
    const ola::rdm::DeviceDescriptor &device) {
  vector<section_info> sections;
  std::set<uint16_t> pids;
  copy(pid_list.begin(), pid_list.end(), inserter(pids, pids.end()));

  // PID_DEVICE_INFO is required so we always add it
  string hint;
  if (pids.find(ola::rdm::PID_DEVICE_MODEL_DESCRIPTION) != pids.end()) {
      hint.push_back('m');  // m is for device model
  }

  AddSection(&sections, DEVICE_INFO_SECTION, DEVICE_INFO_SECTION_NAME, hint);

  AddSection(&sections, IDENTIFY_DEVICE_SECTION, IDENTIFY_DEVICE_SECTION_NAME);

  bool dmx_address_added = false;
  bool include_software_version = false;
  set<uint16_t>::const_iterator iter = pids.begin();
  for (; iter != pids.end(); ++iter) {
    switch (*iter) {
      case ola::rdm::PID_PROXIED_DEVICES:
        AddSection(&sections, PROXIED_DEVICES_SECTION,
                   PROXIED_DEVICES_SECTION_NAME);
        break;
      case ola::rdm::PID_COMMS_STATUS:
        AddSection(&sections, COMMS_STATUS_SECTION, COMMS_STATUS_SECTION_NAME);
        break;
      case ola::rdm::PID_PRODUCT_DETAIL_ID_LIST:
        AddSection(&sections, PRODUCT_DETAIL_SECTION,
                   PRODUCT_DETAIL_SECTION_NAME);
        break;
      case ola::rdm::PID_MANUFACTURER_LABEL:
        AddSection(&sections, MANUFACTURER_LABEL_SECTION,
                   MANUFACTURER_LABEL_SECTION_NAME);
        break;
      case ola::rdm::PID_DEVICE_LABEL:
        AddSection(&sections, DEVICE_LABEL_SECTION, DEVICE_LABEL_SECTION_NAME);
        break;
      case ola::rdm::PID_FACTORY_DEFAULTS:
        AddSection(&sections, FACTORY_DEFAULTS_SECTION,
                   FACTORY_DEFAULTS_SECTION_NAME);
        break;
      case ola::rdm::PID_LANGUAGE:
        AddSection(&sections, LANGUAGE_SECTION, LANGUAGE_SECTION_NAME);
        break;
      case ola::rdm::PID_BOOT_SOFTWARE_VERSION_ID:
      case ola::rdm::PID_BOOT_SOFTWARE_VERSION_LABEL:
        include_software_version = true;
        break;
      case ola::rdm::PID_DMX_PERSONALITY:
        if (pids.find(ola::rdm::PID_DMX_PERSONALITY_DESCRIPTION) ==
            pids.end()) {
          AddSection(&sections, PERSONALITY_SECTION, PERSONALITY_SECTION_NAME);
        } else {
          AddSection(&sections, PERSONALITY_SECTION, PERSONALITY_SECTION_NAME,
                     "l");
        }
        break;
      case ola::rdm::PID_DMX_START_ADDRESS:
        AddSection(&sections, DMX_ADDRESS_SECTION, DMX_ADDRESS_SECTION_NAME);
        dmx_address_added = true;
        break;
      case ola::rdm::PID_DEVICE_HOURS:
        AddSection(&sections, DEVICE_HOURS_SECTION, DEVICE_HOURS_SECTION_NAME);
        break;
      case ola::rdm::PID_LAMP_HOURS:
        AddSection(&sections, LAMP_HOURS_SECTION, LAMP_HOURS_SECTION_NAME);
        break;
      case ola::rdm::PID_LAMP_STRIKES:
        AddSection(&sections, LAMP_STRIKES_SECTION, LAMP_STRIKES_SECTION_NAME);
        break;
      case ola::rdm::PID_LAMP_STATE:
        AddSection(&sections, LAMP_STATE_SECTION, LAMP_STATE_SECTION_NAME);
        break;
      case ola::rdm::PID_LAMP_ON_MODE:
        AddSection(&sections, LAMP_MODE_SECTION, LAMP_MODE_SECTION_NAME);
        break;
      case ola::rdm::PID_DEVICE_POWER_CYCLES:
        AddSection(&sections, POWER_CYCLES_SECTION, POWER_CYCLES_SECTION_NAME);
        break;
      case ola::rdm::PID_DISPLAY_INVERT:
        AddSection(&sections, DISPLAY_INVERT_SECTION,
                   DISPLAY_INVERT_SECTION_NAME);
        break;
      case ola::rdm::PID_DISPLAY_LEVEL:
        AddSection(&sections, DISPLAY_LEVEL_SECTION,
                   DISPLAY_LEVEL_SECTION_NAME);
        break;
      case ola::rdm::PID_PAN_INVERT:
        AddSection(&sections, PAN_INVERT_SECTION, PAN_INVERT_SECTION_NAME);
        break;
      case ola::rdm::PID_TILT_INVERT:
        AddSection(&sections, TILT_INVERT_SECTION, TILT_INVERT_SECTION_NAME);
        break;
      case ola::rdm::PID_PAN_TILT_SWAP:
        AddSection(&sections, PAN_TILT_SWAP_SECTION,
                   PAN_TILT_SWAP_SECTION_NAME);
        break;
      case ola::rdm::PID_REAL_TIME_CLOCK:
        AddSection(&sections, CLOCK_SECTION, CLOCK_SECTION_NAME);
        break;
      case ola::rdm::PID_POWER_STATE:
        AddSection(&sections, POWER_STATE_SECTION, POWER_STATE_SECTION_NAME);
        break;
      case ola::rdm::PID_RESET_DEVICE:
        AddSection(&sections, RESET_DEVICE_SECTION, RESET_DEVICE_SECTION_NAME);
        break;
      case ola::rdm::PID_DNS_HOSTNAME:
        AddSection(&sections, DNS_HOSTNAME_SECTION, DNS_HOSTNAME_SECTION_NAME);
        break;
      case ola::rdm::PID_DNS_DOMAIN_NAME:
        AddSection(&sections,
                   DNS_DOMAIN_NAME_SECTION,
                   DNS_DOMAIN_NAME_SECTION_NAME);
        break;
      case ola::rdm::PID_CURVE:
        AddSection(&sections, CURVE_SECTION, CURVE_SECTION_NAME);
        break;
      case ola::rdm::PID_DIMMER_INFO:
        AddSection(&sections, DIMMER_INFO_SECTION, DIMMER_INFO_SECTION_NAME);
        break;
      case ola::rdm::PID_MINIMUM_LEVEL:
        AddSection(&sections, DIMMER_MINIMUM_SECTION,
                   DIMMER_MINIMUM_SECTION_NAME);
        break;
      case ola::rdm::PID_MAXIMUM_LEVEL:
        AddSection(&sections, DIMMER_MAXIMUM_SECTION,
                   DIMMER_MAXIMUM_SECTION_NAME);
        break;
    }
  }

  if (include_software_version) {
    AddSection(&sections, BOOT_SOFTWARE_SECTION, BOOT_SOFTWARE_SECTION_NAME);
  }

  if (CheckForRDMSuccess(status)) {
    if (device.dmx_footprint && !dmx_address_added) {
      AddSection(&sections, DMX_ADDRESS_SECTION, DMX_ADDRESS_SECTION_NAME);
    }
    if (device.sensor_count &&
        pids.find(ola::rdm::PID_SENSOR_DEFINITION) != pids.end() &&
        pids.find(ola::rdm::PID_SENSOR_VALUE) != pids.end()) {
      // sensors count from 1
      for (unsigned int i = 0; i < device.sensor_count; ++i) {
        ostringstream heading, hint;
        hint << i;
        heading << "Sensor " << std::setfill(' ') << std::setw(3) << i;
        AddSection(&sections, SENSOR_SECTION, heading.str(), hint.str());
      }
    }
  }

  sort(sections.begin(), sections.end(), lt_section_info());

  JsonArray json;
  vector<section_info>::const_iterator section_iter = sections.begin();
  for (; section_iter != sections.end(); ++section_iter) {
    JsonObject *json_obj = json.AppendObject();
    json_obj->Add("id", section_iter->id);
    json_obj->Add("name", section_iter->name);
    json_obj->Add("hint",  section_iter->hint);
  }

  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(json);
  delete response;
}


/*
 * @brief Handle the request for the communication status.
 */
string RDMHTTPModule::GetCommStatus(HTTPResponse *response,
                                    unsigned int universe_id,
                                    const UID &uid) {
  string error;
  m_rdm_api.GetCommStatus(
    universe_id,
    uid,
    NewSingleCallback(this,
                      &RDMHTTPModule::CommStatusHandler,
                      response),
    &error);
  return error;
}


/**
 * @brief Handle the response to a communication status call
 */
void RDMHTTPModule::CommStatusHandler(HTTPResponse *response,
                                      const ola::rdm::ResponseStatus &status,
                                      uint16_t short_messages,
                                      uint16_t length_mismatch,
                                      uint16_t checksum_fail) {
  if (CheckForRDMError(response, status)) {
    return;
  }
  JsonSection section;

  section.AddItem(new UIntItem("Short Messages", short_messages));
  section.AddItem(new UIntItem("Length Mismatch", length_mismatch));
  section.AddItem(new UIntItem("Checksum Failures", checksum_fail));
  section.AddItem(new HiddenItem("1", GENERIC_UINT_FIELD));
  section.SetSaveButton("Clear Counters");
  RespondWithSection(response, section);
}


/**
 * @brief Clear the communication status counters
 */
string RDMHTTPModule::ClearCommsCounters(HTTPResponse *response,
                                         unsigned int universe_id,
                                         const UID &uid) {
  string error;
  m_rdm_api.ClearCommStatus(
      universe_id,
      uid,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/*
 * @brief Handle the request for the proxied devices
 */
string RDMHTTPModule::GetProxiedDevices(HTTPResponse *response,
                                        unsigned int universe_id,
                                        const UID &uid) {
  string error;
  m_rdm_api.GetProxiedDevices(
    universe_id,
    uid,
    NewSingleCallback(this,
                      &RDMHTTPModule::ProxiedDevicesHandler,
                      response,
                      universe_id),
    &error);
  return error;
}


/**
 * @brief Handle the response to a proxied devices call.
 */
void RDMHTTPModule::ProxiedDevicesHandler(
    HTTPResponse *response,
    unsigned int universe_id,
    const ola::rdm::ResponseStatus &status,
    const vector<UID> &uids) {
  if (CheckForRDMError(response, status)) {
    return;
  }
  JsonSection section;

  uid_resolution_state *uid_state = GetUniverseUids(universe_id);
  vector<UID>::const_iterator iter = uids.begin();
  unsigned int i = 1;
  for (; iter != uids.end(); ++iter, ++i) {
    string uid = iter->ToString();

    // attempt to add device & manufacturer names
    if (uid_state) {
      map<UID, resolved_uid>::iterator uid_iter =
        uid_state->resolved_uids.find(*iter);

      if (uid_iter != uid_state->resolved_uids.end()) {
        string device = uid_iter->second.device;
        string manufacturer = uid_iter->second.manufacturer;

        if (!(device.empty() && manufacturer.empty())) {
          ostringstream str;
          str << uid_iter->second.manufacturer;
          if ((!device.empty()) && (!manufacturer.empty())) {
            str << ", ";
          }
          str << uid_iter->second.device;
          str << " [";
          str << iter->ToString();
          str << "]";
          uid = str.str();
        }
      }
    }
    section.AddItem(new StringItem("Device " + IntToString(i), uid));
  }
  RespondWithSection(response, section);
}


/*
 * @brief Handle the request for the device info section.
 */
string RDMHTTPModule::GetDeviceInfo(const HTTPRequest *request,
                                    HTTPResponse *response,
                                    unsigned int universe_id,
                                    const UID &uid) {
  string hint = request->GetParameter(HINT_KEY);
  string error;
  device_info dev_info = {universe_id, uid, hint, "", ""};

  m_rdm_api.GetSoftwareVersionLabel(
    universe_id,
    uid,
    ola::rdm::ROOT_RDM_DEVICE,
    NewSingleCallback(this,
                      &RDMHTTPModule::GetSoftwareVersionHandler,
                      response,
                      dev_info),
    &error);
  return error;
}


/**
 * @brief Handle the response to a software version call.
 */
void RDMHTTPModule::GetSoftwareVersionHandler(
    HTTPResponse *response,
    device_info dev_info,
    const ola::rdm::ResponseStatus &status,
    const string &software_version) {
  string error;

  if (CheckForRDMSuccess(status)) {
    dev_info.software_version = software_version;
  }

  if (dev_info.hint.find('m') != string::npos) {
    m_rdm_api.GetDeviceModelDescription(
        dev_info.universe_id,
        dev_info.uid,
        ola::rdm::ROOT_RDM_DEVICE,
        NewSingleCallback(this,
                          &RDMHTTPModule::GetDeviceModelHandler,
                          response,
                          dev_info),
        &error);
  } else {
    m_rdm_api.GetDeviceInfo(
        dev_info.universe_id,
        dev_info.uid,
        ola::rdm::ROOT_RDM_DEVICE,
        NewSingleCallback(this,
                          &RDMHTTPModule::GetDeviceInfoHandler,
                          response,
                          dev_info),
        &error);
  }

  if (!error.empty()) {
    m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR + error);
  }
}


/**
 * @brief Handle the response to a device model call.
 */
void RDMHTTPModule::GetDeviceModelHandler(
    HTTPResponse *response,
    device_info dev_info,
    const ola::rdm::ResponseStatus &status,
    const string &device_model) {
  string error;

  if (CheckForRDMSuccess(status)) {
    dev_info.device_model = device_model;
  }

  m_rdm_api.GetDeviceInfo(
      dev_info.universe_id,
      dev_info.uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GetDeviceInfoHandler,
                        response,
                        dev_info),
      &error);

  if (!error.empty()) {
    m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR + error);
  }
}


/**
 * @brief Handle the response to a device info call and build the response
 */
void RDMHTTPModule::GetDeviceInfoHandler(
    HTTPResponse *response,
    device_info dev_info,
    const ola::rdm::ResponseStatus &status,
    const ola::rdm::DeviceDescriptor &device) {
  JsonSection section;

  if (CheckForRDMError(response, status)) {
    return;
  }

  ostringstream stream;
  stream << static_cast<int>(device.protocol_version_high) << "."
         << static_cast<int>(device.protocol_version_low);
  section.AddItem(new StringItem("Protocol Version", stream.str()));

  stream.str("");
  if (dev_info.device_model.empty()) {
    stream << device.device_model;
  } else {
    stream << dev_info.device_model << " (" << device.device_model << ")";
  }
  section.AddItem(new StringItem("Device Model", stream.str()));

  section.AddItem(new StringItem(
      "Product Category",
      ola::rdm::ProductCategoryToString(device.product_category)));
  stream.str("");
  if (dev_info.software_version.empty()) {
    stream << device.software_version;
  } else {
    stream << dev_info.software_version << " (" << device.software_version
           << ")";
  }
  section.AddItem(new StringItem("Software Version", stream.str()));

  if (device.dmx_start_address == ola::rdm::ZERO_FOOTPRINT_DMX_ADDRESS) {
    section.AddItem(new StringItem("DMX Address", "N/A"));
  } else {
    section.AddItem(new UIntItem("DMX Address", device.dmx_start_address));
  }

  section.AddItem(new UIntItem("DMX Footprint", device.dmx_footprint));

  stream.str("");
  stream << static_cast<int>(device.current_personality) << " of "
         << static_cast<int>(device.personality_count);
  section.AddItem(new StringItem("Personality", stream.str()));

  section.AddItem(new UIntItem("Sub Devices", device.sub_device_count));
  section.AddItem(new UIntItem("Sensors", device.sensor_count));
  section.AddItem(new StringItem("UID", dev_info.uid.ToString()));
  RespondWithSection(response, section);
}


/*
 * @brief Handle the request for the product details ids.
 */
string RDMHTTPModule::GetProductIds(OLA_UNUSED const HTTPRequest *request,
                                    HTTPResponse *response,
                                    unsigned int universe_id,
                                    const UID &uid) {
  string error;
  m_rdm_api.GetProductDetailIdList(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GetProductIdsHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the response to a product detail ids call and build the response.
 */
void RDMHTTPModule::GetProductIdsHandler(
    HTTPResponse *response,
    const ola::rdm::ResponseStatus &status,
    const vector<uint16_t> &ids) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  bool first = true;
  ostringstream product_ids;
  JsonSection section;
  vector<uint16_t>::const_iterator iter = ids.begin();
  for (; iter != ids.end(); ++iter) {
    string product_id = ola::rdm::ProductDetailToString(*iter);
    if (product_id.empty()) {
      continue;
    }

    if (first) {
      first = false;
    } else {
      product_ids << ", ";
    }
    product_ids << product_id;
  }
  section.AddItem(new StringItem("Product Detail IDs", product_ids.str()));
  RespondWithSection(response, section);
}


/**
 * @brief Handle the request for the Manufacturer label.
 */
string RDMHTTPModule::GetManufacturerLabel(
    OLA_UNUSED const HTTPRequest *request,
    HTTPResponse *response,
    unsigned int universe_id,
    const UID &uid) {
  string error;
  m_rdm_api.GetManufacturerLabel(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GetManufacturerLabelHandler,
                        response,
                        universe_id,
                        uid),
      &error);
  return error;
}


/**
 * @brief Handle the response to a manufacturer label call and build the response
 */
void RDMHTTPModule::GetManufacturerLabelHandler(
    HTTPResponse *response,
    unsigned int universe_id,
    const UID uid,
    const ola::rdm::ResponseStatus &status,
    const string &label) {
  if (CheckForRDMError(response, status)) {
    return;
  }
  JsonSection section;
  section.AddItem(new StringItem("Manufacturer Label", label));
  RespondWithSection(response, section);

  // update the map as well
  uid_resolution_state *uid_state = GetUniverseUids(universe_id);
  if (uid_state) {
    map<UID, resolved_uid>::iterator uid_iter =
      uid_state->resolved_uids.find(uid);
    if (uid_iter != uid_state->resolved_uids.end()) {
      uid_iter->second.manufacturer = label;
    }
  }
}


/**
 * @brief Handle the request for the Device label.
 */
string RDMHTTPModule::GetDeviceLabel(OLA_UNUSED const HTTPRequest *request,
                                     HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string error;
  m_rdm_api.GetDeviceLabel(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GetDeviceLabelHandler,
                        response,
                        universe_id,
                        uid),
      &error);
  return error;
}


/**
 * @brief Handle the response to a device label call and build the response
 */
void RDMHTTPModule::GetDeviceLabelHandler(
    HTTPResponse *response,
    unsigned int universe_id,
    const UID uid,
    const ola::rdm::ResponseStatus &status,
    const string &label) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  section.AddItem(new StringItem("Device Label", label, LABEL_FIELD));
  RespondWithSection(response, section);

  // update the map as well
  uid_resolution_state *uid_state = GetUniverseUids(universe_id);
  if (uid_state) {
    map<UID, resolved_uid>::iterator uid_iter =
      uid_state->resolved_uids.find(uid);
    if (uid_iter != uid_state->resolved_uids.end()) {
      uid_iter->second.device = label;
    }
  }
}


/*
 * @brief Set the device label
 */
string RDMHTTPModule::SetDeviceLabel(const HTTPRequest *request,
                                     HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string label = request->GetParameter(LABEL_FIELD);
  string error;
  m_rdm_api.SetDeviceLabel(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      label,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the factory defaults section
 */
string RDMHTTPModule::GetFactoryDefaults(HTTPResponse *response,
                                         unsigned int universe_id,
                                         const UID &uid) {
  string error;
  m_rdm_api.GetFactoryDefaults(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::FactoryDefaultsHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the response to a factory defaults call and build the response
 */
void RDMHTTPModule::FactoryDefaultsHandler(
    HTTPResponse *response,
    const ola::rdm::ResponseStatus &status,
    bool defaults) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  section.AddItem(new StringItem("Using Defaults",
                                 defaults ? "Yes" : "No"));
  section.AddItem(new HiddenItem("1", GENERIC_UINT_FIELD));
  section.SetSaveButton("Reset to Defaults");
  RespondWithSection(response, section);
}


/*
 * @brief Reset to the factory defaults
 */
string RDMHTTPModule::SetFactoryDefault(HTTPResponse *response,
                                        unsigned int universe_id,
                                        const UID &uid) {
  string error;
  m_rdm_api.ResetToFactoryDefaults(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the language section.
 */
string RDMHTTPModule::GetLanguage(HTTPResponse *response,
                                  unsigned int universe_id,
                                  const UID &uid) {
  string error;
  m_rdm_api.GetLanguageCapabilities(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GetSupportedLanguagesHandler,
                        response,
                        universe_id,
                        uid),
      &error);
  return error;
}


/**
 * @brief Handle the response to language capability call.
 */
void RDMHTTPModule::GetSupportedLanguagesHandler(
    HTTPResponse *response,
    unsigned int universe_id,
    const UID uid,
    OLA_UNUSED const ola::rdm::ResponseStatus &status,
    const vector<string> &languages) {
  string error;
  m_rdm_api.GetLanguage(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GetLanguageHandler,
                        response,
                        languages),
      &error);

  if (!error.empty()) {
    m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR + error);
  }
}


/**
 * @brief Handle the response to language call and build the response
 */
void RDMHTTPModule::GetLanguageHandler(HTTPResponse *response,
                                       vector<string> languages,
                                       const ola::rdm::ResponseStatus &status,
                                       const string &language) {
  JsonSection section;
  SelectItem *item = new SelectItem("Language", LANGUAGE_FIELD);
  bool ok = CheckForRDMSuccess(status);

  vector<string>::const_iterator iter = languages.begin();
  unsigned int i = 0;
  for (; iter != languages.end(); ++iter, i++) {
    item->AddItem(*iter, *iter);
    if (ok && *iter == language) {
      item->SetSelectedOffset(i);
    }
  }

  if (ok && languages.empty()) {
    item->AddItem(language, language);
    item->SetSelectedOffset(0);
  }
  section.AddItem(item);
  RespondWithSection(response, section);
}


/*
 * @brief Set the language
 */
string RDMHTTPModule::SetLanguage(const HTTPRequest *request,
                                  HTTPResponse *response,
                                  unsigned int universe_id,
                                  const UID &uid) {
  string label = request->GetParameter(LANGUAGE_FIELD);
  string error;
  m_rdm_api.SetLanguage(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      label,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the boot software section.
 */
string RDMHTTPModule::GetBootSoftware(HTTPResponse *response,
                                      unsigned int universe_id,
                                      const UID &uid) {
  string error;
  m_rdm_api.GetBootSoftwareVersionLabel(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GetBootSoftwareLabelHandler,
                        response,
                        universe_id,
                        uid),
      &error);
  return error;
}


/**
 * @brief Handle the response to a boot software label.
 */
void RDMHTTPModule::GetBootSoftwareLabelHandler(
    HTTPResponse *response,
    unsigned int universe_id,
    const UID uid,
    OLA_UNUSED const ola::rdm::ResponseStatus &status,
    const string &label) {
  string error;
  m_rdm_api.GetBootSoftwareVersion(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GetBootSoftwareVersionHandler,
                        response,
                        label),
      &error);
  if (!error.empty()) {
    m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR + error);
  }
}


/**
 * @brief Handle the response to a boot software version.
 */
void RDMHTTPModule::GetBootSoftwareVersionHandler(
    HTTPResponse *response,
    string label,
    const ola::rdm::ResponseStatus &status,
    uint32_t version) {
  ostringstream str;
  str << label;
  if (CheckForRDMSuccess(status)) {
    if (!label.empty()) {
      str << " (" << version << ")";
    } else {
      str << version;
    }
  }

  JsonSection section;
  StringItem *item = new StringItem("Boot Software", str.str());
  section.AddItem(item);
  RespondWithSection(response, section);
}


/**
 * @brief Handle the request for the personality section.
 */
string RDMHTTPModule::GetPersonalities(OLA_UNUSED const HTTPRequest *request,
                                       HTTPResponse *response,
                                       unsigned int universe_id,
                                       const UID &uid,
                                       bool return_as_section,
                                       bool include_descriptions) {
  string hint = request->GetParameter(HINT_KEY);
  string error;

  personality_info *info = new personality_info;
  info->universe_id = universe_id;
  info->uid = new UID(uid);
  info->include_descriptions = include_descriptions || (hint == "l");
  info->return_as_section = return_as_section;
  info->active = 0;
  info->next = 1;
  info->total = 0;

  m_rdm_api.GetDMXPersonality(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GetPersonalityHandler,
                        response,
                        info),
      &error);
  return error;
}


/**
 * @brief Handle the response to a dmx personality call.
 */
void RDMHTTPModule::GetPersonalityHandler(
    HTTPResponse *response,
    personality_info *info,
    const ola::rdm::ResponseStatus &status,
    uint8_t current,
    uint8_t total) {
  if (CheckForRDMError(response, status)) {
    delete info->uid;
    delete info;
    return;
  }

  info->active = current;
  info->total = total;

  if (info->include_descriptions) {
    GetNextPersonalityDescription(response, info);
  } else {
    SendPersonalityResponse(response, info);
  }
}


/**
 * @brief Get the description of the next dmx personality
 */
void RDMHTTPModule::GetNextPersonalityDescription(HTTPResponse *response,
                                                  personality_info *info) {
  string error;
  while (info->next <= info->total) {
    bool r = m_rdm_api.GetDMXPersonalityDescription(
        info->universe_id,
        *(info->uid),
        ola::rdm::ROOT_RDM_DEVICE,
        info->next,
        NewSingleCallback(this,
                          &RDMHTTPModule::GetPersonalityLabelHandler,
                          response,
                          info),
        &error);
    if (r) {
      return;
    }

    info->next++;
  }
  if (info->return_as_section) {
    SendSectionPersonalityResponse(response, info);
  } else {
    SendPersonalityResponse(response, info);
  }
}


/**
 * @brief Handle the response to a Personality label call.
 *
 * This fetches the next personality in the sequence, or sends the response if
 * we have all the info.
 */
void RDMHTTPModule::GetPersonalityLabelHandler(
    HTTPResponse *response,
    personality_info *info,
    const ola::rdm::ResponseStatus &status,
    OLA_UNUSED uint8_t personality,
    uint16_t slot_count,
    const string &label) {
  string description = "";
  uint32_t slots = INVALID_PERSONALITY;

  if (CheckForRDMSuccess(status)) {
    slots = slot_count;
    description = label;
  }

  info->personalities.push_back(pair<uint32_t, string>(slots, description));

  if (info->next == info->total) {
    if (info->return_as_section) {
      SendSectionPersonalityResponse(response, info);
    } else {
      SendPersonalityResponse(response, info);
    }
  } else {
    info->next++;
    GetNextPersonalityDescription(response, info);
  }
}


/**
 * @brief Send the response to a dmx personality section
 */
void RDMHTTPModule::SendSectionPersonalityResponse(HTTPResponse *response,
                                                   personality_info *info) {
  JsonSection section;
  SelectItem *item = new SelectItem("Personality", GENERIC_UINT_FIELD);

  for (unsigned int i = 1; i <= info->total; i++) {
    if (i <= info->personalities.size() &&
        info->personalities[i - 1].first != INVALID_PERSONALITY) {
      ostringstream str;
      str << info->personalities[i - 1].second << " (" <<
        info->personalities[i - 1].first << ")";
      item->AddItem(str.str(), i);
    } else {
      item->AddItem(IntToString(i), i);
    }

    if (info->active == i) {
      item->SetSelectedOffset(i - 1);
    }
  }
  section.AddItem(item);
  RespondWithSection(response, section);

  delete info->uid;
  delete info;
}


/**
 * @brief Set the personality
 */
string RDMHTTPModule::SetPersonality(const HTTPRequest *request,
                                     HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string personality_str = request->GetParameter(GENERIC_UINT_FIELD);
  uint8_t personality;

  if (!StringToInt(personality_str, &personality)) {
    return "Invalid personality";
  }

  string error;
  m_rdm_api.SetDMXPersonality(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      personality,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the start address section.
 */
string RDMHTTPModule::GetStartAddress(OLA_UNUSED const HTTPRequest *request,
                                      HTTPResponse *response,
                                      unsigned int universe_id,
                                      const UID &uid) {
  string error;
  m_rdm_api.GetDMXAddress(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GetStartAddressHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the response to a dmx start address call and build the response
 */
void RDMHTTPModule::GetStartAddressHandler(
    HTTPResponse *response,
    const ola::rdm::ResponseStatus &status,
    uint16_t address) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  GenericItem *item = NULL;
  if (address == ola::rdm::ZERO_FOOTPRINT_DMX_ADDRESS) {
    item = new StringItem("DMX Start Address", "N/A");
  } else {
    UIntItem *uint_item = new UIntItem("DMX Start Address", address,
                                       ADDRESS_FIELD);
    uint_item->SetMin(DMX_MIN_SLOT_NUMBER);
    uint_item->SetMax(DMX_MAX_SLOT_NUMBER);
    item = uint_item;
  }
  section.AddItem(item);
  RespondWithSection(response, section);
}


/*
 * @brief Set the DMX start address
 */
string RDMHTTPModule::SetStartAddress(const HTTPRequest *request,
                                      HTTPResponse *response,
                                      unsigned int universe_id,
                                      const UID &uid) {
  string dmx_address = request->GetParameter(ADDRESS_FIELD);
  uint16_t address;

  if (!StringToInt(dmx_address, &address)) {
    return "Invalid start address";
  }

  string error;
  m_rdm_api.SetDMXAddress(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      address,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the sensor section.
 */
string RDMHTTPModule::GetSensor(const HTTPRequest *request,
                                HTTPResponse *response,
                                unsigned int universe_id,
                                const UID &uid) {
  string hint = request->GetParameter(HINT_KEY);
  uint8_t sensor_id;
  if (!StringToInt(hint, &sensor_id)) {
    return "Invalid hint (sensor #)";
  }

  string error;
  m_rdm_api.GetSensorDefinition(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      sensor_id,
      NewSingleCallback(this,
                        &RDMHTTPModule::SensorDefinitionHandler,
                        response,
                        universe_id,
                        uid,
                        sensor_id),
      &error);
  return error;
}


/**
 * @brief Handle the response to a sensor definition request.
 */
void RDMHTTPModule::SensorDefinitionHandler(
    HTTPResponse *response,
    unsigned int universe_id,
    const UID uid,
    uint8_t sensor_id,
    const ola::rdm::ResponseStatus &status,
    const ola::rdm::SensorDescriptor &definition) {
  ola::rdm::SensorDescriptor *definition_arg = NULL;

  if (CheckForRDMSuccess(status)) {
    definition_arg = new ola::rdm::SensorDescriptor();
    *definition_arg = definition;
  }
  string error;
  m_rdm_api.GetSensorValue(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      sensor_id,
      NewSingleCallback(this,
                        &RDMHTTPModule::SensorValueHandler,
                        response,
                        definition_arg),
      &error);
  if (!error.empty()) {
    m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR + error);
  }
}


/**
 * @brief Handle the response to a sensor value request & build the response.
 */
void RDMHTTPModule::SensorValueHandler(
    HTTPResponse *response,
    ola::rdm::SensorDescriptor *definition,
    const ola::rdm::ResponseStatus &status,
    const ola::rdm::SensorValueDescriptor &value) {
  if (CheckForRDMError(response, status)) {
    if (definition) {
      delete definition;
    }
    return;
  }

  JsonSection section;
  ostringstream str;

  if (definition) {
    section.AddItem(new StringItem("Description", definition->description));
  }

  str << value.present_value;
  if (definition) {
    str << " " << ola::rdm::PrefixToString(definition->prefix) << " "
        << ola::rdm::UnitToString(definition->unit);
  }
  section.AddItem(new StringItem("Present Value", str.str()));

  if (definition) {
    section.AddItem(new StringItem(
          "Type",
          ola::rdm::SensorTypeToString(definition->type)));
    str.str("");
    str << definition->range_min << " - " << definition->range_max << " "
        << ola::rdm::PrefixToString(definition->prefix) << " " <<
      ola::rdm::UnitToString(definition->unit);
    section.AddItem(new StringItem("Range", str.str()));

    str.str("");
    str << definition->normal_min << " - " << definition->normal_max << " "
        << ola::rdm::PrefixToString(definition->prefix) << " " <<
      ola::rdm::UnitToString(definition->unit);
    section.AddItem(new StringItem("Normal Range", str.str()));

    if (definition->recorded_value_support &
        ola::rdm::SENSOR_RECORDED_VALUE) {
      str.str("");
      str << value.recorded << " "
          << ola::rdm::PrefixToString(definition->prefix) << " "
          << ola::rdm::UnitToString(definition->unit);
      section.AddItem(new StringItem("Recorded Value", str.str()));
    }

    if (definition->recorded_value_support &
        ola::rdm::SENSOR_RECORDED_RANGE_VALUES) {
      str.str("");
      str << value.lowest << " - " << value.highest << " "
          << ola::rdm::PrefixToString(definition->prefix) << " "
          << ola::rdm::UnitToString(definition->unit);
      section.AddItem(new StringItem("Min / Max Recorded Values", str.str()));
    }
  }

  if (definition && definition->recorded_value_support) {
    section.AddItem(new HiddenItem("1", RECORD_SENSOR_FIELD));
  }
  section.SetSaveButton("Record Sensor");
  RespondWithSection(response, section);
  delete definition;
}


/*
 * @brief Record a sensor value
 */
string RDMHTTPModule::RecordSensor(const HTTPRequest *request,
                                   HTTPResponse *response,
                                   unsigned int universe_id,
                                   const UID &uid) {
  string hint = request->GetParameter(HINT_KEY);
  uint8_t sensor_id;
  if (!StringToInt(hint, &sensor_id)) {
    return "Invalid hint (sensor #)";
  }

  string error;
  m_rdm_api.RecordSensors(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      sensor_id,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the device hours section.
 */
string RDMHTTPModule::GetDeviceHours(OLA_UNUSED const HTTPRequest *request,
                                     HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string error;
  m_rdm_api.GetDeviceHours(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GenericUIntHandler,
                        response,
                        string("Device Hours")),
      &error);
  return error;
}


/**
 * @brief Set the device hours
 */
string RDMHTTPModule::SetDeviceHours(const HTTPRequest *request,
                                     HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string device_hours = request->GetParameter(GENERIC_UINT_FIELD);
  uint32_t dev_hours;

  if (!StringToInt(device_hours, &dev_hours)) {
    return "Invalid device hours";
  }

  string error;
  m_rdm_api.SetDeviceHours(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      dev_hours,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the lamp hours section.
 */
string RDMHTTPModule::GetLampHours(OLA_UNUSED const HTTPRequest *request,
                                   HTTPResponse *response,
                                   unsigned int universe_id,
                                   const UID &uid) {
  string error;
  m_rdm_api.GetLampHours(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GenericUIntHandler,
                        response,
                        string("Lamp Hours")),
      &error);
  return error;
}


/**
 * @brief Set the lamp hours
 */
string RDMHTTPModule::SetLampHours(const HTTPRequest *request,
                                   HTTPResponse *response,
                                   unsigned int universe_id,
                                   const UID &uid) {
  string lamp_hours_str = request->GetParameter(GENERIC_UINT_FIELD);
  uint32_t lamp_hours;

  if (!StringToInt(lamp_hours_str, &lamp_hours)) {
    return "Invalid lamp hours";
  }

  string error;
  m_rdm_api.SetLampHours(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      lamp_hours,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the lamp strikes section
 */
string RDMHTTPModule::GetLampStrikes(OLA_UNUSED const HTTPRequest *request,
                                     HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string error;
  m_rdm_api.GetLampStrikes(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GenericUIntHandler,
                        response,
                        string("Lamp Strikes")),
      &error);
  return error;
}


/**
 * @brief Set the lamp strikes
 */
string RDMHTTPModule::SetLampStrikes(const HTTPRequest *request,
                                     HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string lamp_strikes_str = request->GetParameter(GENERIC_UINT_FIELD);
  uint32_t lamp_strikes;

  if (!StringToInt(lamp_strikes_str, &lamp_strikes)) {
    return "Invalid lamp strikes";
  }

  string error;
  m_rdm_api.SetLampStrikes(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      lamp_strikes,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the lamp state section
 */
string RDMHTTPModule::GetLampState(OLA_UNUSED const HTTPRequest *request,
                                   HTTPResponse *response,
                                   unsigned int universe_id,
                                   const UID &uid) {
  string error;
  m_rdm_api.GetLampState(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::LampStateHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the response to lamp state call and build the response
 */
void RDMHTTPModule::LampStateHandler(HTTPResponse *response,
                                     const ola::rdm::ResponseStatus &status,
                                     uint8_t state) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  SelectItem *item = new SelectItem("Lamp State", GENERIC_UINT_FIELD);

  typedef struct {
    string label;
    ola::rdm::rdm_lamp_state state;
  } values_s;

  values_s possible_values[] = {
    {"Off", ola::rdm::LAMP_OFF},
    {"On", ola::rdm::LAMP_ON},
    {"Strike", ola::rdm::LAMP_STRIKE},
    {"Standby", ola::rdm::LAMP_STANDBY}};

  for (unsigned int i = 0; i < sizeof(possible_values) / sizeof(values_s);
       ++i) {
    item->AddItem(possible_values[i].label, possible_values[i].state);
    if (state == possible_values[i].state) {
      item->SetSelectedOffset(i);
    }
  }

  section.AddItem(item);
  RespondWithSection(response, section);
}


/**
 * @brief Set the lamp state
 */
string RDMHTTPModule::SetLampState(const HTTPRequest *request,
                                   HTTPResponse *response,
                                   unsigned int universe_id,
                                   const UID &uid) {
  string lamp_state_str = request->GetParameter(GENERIC_UINT_FIELD);
  uint8_t lamp_state;
  if (!StringToInt(lamp_state_str, &lamp_state)) {
    return "Invalid lamp state";
  }

  string error;
  m_rdm_api.SetLampState(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      lamp_state,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the lamp mode section
 */
string RDMHTTPModule::GetLampMode(OLA_UNUSED const HTTPRequest *request,
                                  HTTPResponse *response,
                                  unsigned int universe_id,
                                  const UID &uid) {
  string error;
  m_rdm_api.GetLampMode(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::LampModeHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the response to lamp mode call and build the response
 */
void RDMHTTPModule::LampModeHandler(HTTPResponse *response,
                                    const ola::rdm::ResponseStatus &status,
                                    uint8_t mode) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  SelectItem *item = new SelectItem("Lamp Mode", GENERIC_UINT_FIELD);

  typedef struct {
    string label;
    ola::rdm::rdm_lamp_mode mode;
  } values_s;

  values_s possible_values[] = {
    {"Off", ola::rdm::LAMP_ON_MODE_OFF},
    {"DMX", ola::rdm::LAMP_ON_MODE_DMX},
    {"On", ola::rdm::LAMP_ON_MODE_ON},
    {"On After Calibration", ola::rdm::LAMP_ON_MODE_ON_AFTER_CAL}};

  for (unsigned int i = 0; i < sizeof(possible_values) / sizeof(values_s);
       ++i) {
    item->AddItem(possible_values[i].label, possible_values[i].mode);
    if (mode == possible_values[i].mode) {
      item->SetSelectedOffset(i);
    }
  }

  section.AddItem(item);
  RespondWithSection(response, section);
}


/**
 * @brief Set the lamp mode
 */
string RDMHTTPModule::SetLampMode(const HTTPRequest *request,
                                  HTTPResponse *response,
                                  unsigned int universe_id,
                                  const UID &uid) {
  string lamp_mode_str = request->GetParameter(GENERIC_UINT_FIELD);
  uint8_t lamp_mode;
  if (!StringToInt(lamp_mode_str, &lamp_mode)) {
    return "Invalid lamp mode";
  }

  string error;
  m_rdm_api.SetLampMode(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      lamp_mode,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the device power cycles section
 */
string RDMHTTPModule::GetPowerCycles(OLA_UNUSED const HTTPRequest *request,
                                     HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string error;
  m_rdm_api.GetDevicePowerCycles(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GenericUIntHandler,
                        response,
                        string("Device Power Cycles")),
      &error);
  return error;
}


/**
 * @brief Set the device power cycles
 */
string RDMHTTPModule::SetPowerCycles(const HTTPRequest *request,
                                     HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string power_cycles_str = request->GetParameter(GENERIC_UINT_FIELD);
  uint32_t power_cycles;

  if (!StringToInt(power_cycles_str, &power_cycles)) {
    return "Invalid power cycles";
  }

  string error;
  m_rdm_api.SetDevicePowerCycles(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      power_cycles,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}



/**
 * @brief Handle the request for the display invert section.
 */
string RDMHTTPModule::GetDisplayInvert(HTTPResponse *response,
                                       unsigned int universe_id,
                                       const UID &uid) {
  string error;
  m_rdm_api.GetDisplayInvert(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::DisplayInvertHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the response to display invert call and build the response
 */
void RDMHTTPModule::DisplayInvertHandler(
    HTTPResponse *response,
    const ola::rdm::ResponseStatus &status,
    uint8_t value) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  SelectItem *item = new SelectItem("Display Invert", DISPLAY_INVERT_FIELD);

  item->AddItem("Off", ola::rdm::DISPLAY_INVERT_OFF);
  item->AddItem("On", ola::rdm::DISPLAY_INVERT_ON);
  item->AddItem("Auto", ola::rdm::DISPLAY_INVERT_AUTO);

  if (value < ola::rdm::DISPLAY_INVERT_MAX) {
    item->SetSelectedOffset(value);
  }

  section.AddItem(item);
  RespondWithSection(response, section);
}


/**
 * @brief Set the display invert.
 */
string RDMHTTPModule::SetDisplayInvert(const HTTPRequest *request,
                                       HTTPResponse *response,
                                       unsigned int universe_id,
                                       const UID &uid) {
  string invert_field = request->GetParameter(DISPLAY_INVERT_FIELD);
  uint8_t display_invert;
  if (!StringToInt(invert_field, &display_invert)) {
    return "Invalid display invert";
  }

  string error;
  m_rdm_api.SetDisplayInvert(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      display_invert,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the display level section.
 */
string RDMHTTPModule::GetDisplayLevel(HTTPResponse *response,
                                      unsigned int universe_id,
                                      const UID &uid) {
  string error;
  m_rdm_api.GetDisplayLevel(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::DisplayLevelHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the response to display level call and build the response
 */
void RDMHTTPModule::DisplayLevelHandler(HTTPResponse *response,
                                        const ola::rdm::ResponseStatus &status,
                                        uint8_t value) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  UIntItem *item = new UIntItem("Display Level", value, GENERIC_UINT_FIELD);
  item->SetMin(std::numeric_limits<uint8_t>::min());
  item->SetMax(std::numeric_limits<uint8_t>::max());

  section.AddItem(item);
  RespondWithSection(response, section);
}


/**
 * @brief Set the display level.
 */
string RDMHTTPModule::SetDisplayLevel(const HTTPRequest *request,
                                      HTTPResponse *response,
                                      unsigned int universe_id,
                                      const UID &uid) {
  string display_level_str = request->GetParameter(GENERIC_UINT_FIELD);
  uint8_t display_level;
  if (!StringToInt(display_level_str, &display_level)) {
    return "Invalid display level";
  }

  string error;
  m_rdm_api.SetDisplayLevel(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      display_level,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the pan invert section.
 */
string RDMHTTPModule::GetPanInvert(HTTPResponse *response,
                                   unsigned int universe_id,
                                   const UID &uid) {
  string error;
  m_rdm_api.GetPanInvert(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GenericUInt8BoolHandler,
                        response,
                        string("Pan Invert")),
      &error);
  return error;
}


/**
 * @brief Set the pan invert.
 */
string RDMHTTPModule::SetPanInvert(const HTTPRequest *request,
                                   HTTPResponse *response,
                                   unsigned int universe_id,
                                   const UID &uid) {
  string mode = request->GetParameter(GENERIC_BOOL_FIELD);

  if (mode.empty()) {
    return "Invalid mode value";
  }

  string error;
  m_rdm_api.SetPanInvert(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      mode == "1",
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the tilt invert section.
 */
string RDMHTTPModule::GetTiltInvert(HTTPResponse *response,
                                    unsigned int universe_id,
                                    const UID &uid) {
  string error;
  m_rdm_api.GetTiltInvert(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GenericUInt8BoolHandler,
                        response,
                        string("Tilt Invert")),
      &error);
  return error;
}


/**
 * @brief Set the tilt invert.
 */
string RDMHTTPModule::SetTiltInvert(const HTTPRequest *request,
                                    HTTPResponse *response,
                                    unsigned int universe_id,
                                    const UID &uid) {
  string mode = request->GetParameter(GENERIC_BOOL_FIELD);

  if (mode.empty()) {
    return "Invalid mode value";
  }

  string error;
  m_rdm_api.SetTiltInvert(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      mode == "1",
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the pan/tilt swap section.
 */
string RDMHTTPModule::GetPanTiltSwap(HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string error;
  m_rdm_api.GetPanTiltSwap(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GenericUInt8BoolHandler,
                        response,
                        string("Pan Tilt Swap")),
      &error);
  return error;
}


/**
 * @brief Set the pan/tilt swap.
 */
string RDMHTTPModule::SetPanTiltSwap(const HTTPRequest *request,
                                     HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string mode = request->GetParameter(GENERIC_BOOL_FIELD);

  if (mode.empty()) {
    return "Invalid mode value";
  }

  string error;
  m_rdm_api.SetPanTiltSwap(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      mode == "1",
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the clock section.
 */
string RDMHTTPModule::GetClock(HTTPResponse *response,
                               unsigned int universe_id,
                               const UID &uid) {
  string error;
  m_rdm_api.GetClock(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::ClockHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the response to clock call and build the response
 */
void RDMHTTPModule::ClockHandler(HTTPResponse *response,
                                 const ola::rdm::ResponseStatus &status,
                                 const ola::rdm::ClockValue &clock) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  ostringstream str;
  str << std::setfill('0') << std::setw(2) << static_cast<int>(clock.hour)
      << ":" << std::setw(2) << static_cast<int>(clock.minute) << ":"
      << std::setw(2) << static_cast<int>(clock.second) << " "
      << static_cast<int>(clock.day) << "/" << static_cast<int>(clock.month)
      << "/" << clock.year;

  section.AddItem(new StringItem("Clock", str.str()));
  section.AddItem(new HiddenItem("1", GENERIC_UINT_FIELD));
  section.SetSaveButton("Sync to Server");
  RespondWithSection(response, section);
}


/**
 * @brief Sync the clock
 */
string RDMHTTPModule::SyncClock(HTTPResponse *response,
                                unsigned int universe_id,
                                const UID &uid) {
  time_t now = time(NULL);
  struct tm now_tm;
#ifdef _WIN32
  memcpy(&now_tm, localtime(&now), sizeof(now_tm));
#else
  localtime_r(&now, &now_tm);
#endif  // _WIN32
  ola::rdm::ClockValue clock_value;

  clock_value.year = now_tm.tm_year + 1900;
  clock_value.month = now_tm.tm_mon + 1;
  clock_value.day = now_tm.tm_mday;
  clock_value.hour = now_tm.tm_hour;
  clock_value.minute = now_tm.tm_min;
  clock_value.second = now_tm.tm_sec;
  string error;
  m_rdm_api.SetClock(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      clock_value,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the identify device section.
 */
string RDMHTTPModule::GetIdentifyDevice(HTTPResponse *response,
                                        unsigned int universe_id,
                                        const UID &uid) {
  string error;
  m_rdm_api.GetIdentifyDevice(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GenericBoolHandler,
                        response,
                        string("Identify Device")),
      &error);
  return error;
}


/*
 * @brief Set identify device
 */
string RDMHTTPModule::SetIdentifyDevice(const HTTPRequest *request,
                                        HTTPResponse *response,
                                        unsigned int universe_id,
                                        const UID &uid) {
  string mode = request->GetParameter(GENERIC_BOOL_FIELD);

  if (mode.empty()) {
    return "Invalid mode value";
  }

  string error;
  m_rdm_api.IdentifyDevice(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      mode == "1",
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the power state section.
 */
string RDMHTTPModule::GetPowerState(HTTPResponse *response,
                                    unsigned int universe_id,
                                    const UID &uid) {
  string error;
  m_rdm_api.GetPowerState(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::PowerStateHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the response to power state call and build the response
 */
void RDMHTTPModule::PowerStateHandler(HTTPResponse *response,
                                      const ola::rdm::ResponseStatus &status,
                                      uint8_t value) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  SelectItem *item = new SelectItem("Power State", GENERIC_UINT_FIELD);

  typedef struct {
    string label;
    ola::rdm::rdm_power_state state;
  } values_s;

  values_s possible_values[] = {
    {"Full Off", ola::rdm::POWER_STATE_FULL_OFF},
    {"Shutdown", ola::rdm::POWER_STATE_SHUTDOWN},
    {"Standby", ola::rdm::POWER_STATE_STANDBY},
    {"Normal", ola::rdm::POWER_STATE_NORMAL}};

  for (unsigned int i = 0; i < sizeof(possible_values) / sizeof(values_s);
       ++i) {
    item->AddItem(possible_values[i].label, possible_values[i].state);
    if (value == possible_values[i].state) {
      item->SetSelectedOffset(i);
    }
  }

  section.AddItem(item);
  RespondWithSection(response, section);
}


/*
 * @brief Set the power state.
 */
string RDMHTTPModule::SetPowerState(const HTTPRequest *request,
                                    HTTPResponse *response,
                                    unsigned int universe_id,
                                    const UID &uid) {
  string power_state_str = request->GetParameter(GENERIC_UINT_FIELD);
  uint8_t power_state;
  ola::rdm::rdm_power_state power_state_enum;
  if (!StringToInt(power_state_str, &power_state) ||
      !ola::rdm::UIntToPowerState(power_state, &power_state_enum)) {
    return "Invalid power state";
  }

  string error;
  m_rdm_api.SetPowerState(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      power_state_enum,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the device reset section.
 */
string RDMHTTPModule::GetResetDevice(HTTPResponse *response) {
  JsonSection section = JsonSection(false);
  SelectItem *item = new SelectItem("Reset Device", GENERIC_UINT_FIELD);

  typedef struct {
    string label;
    ola::rdm::rdm_reset_device_mode state;
  } values_s;

  values_s possible_values[] = {
    {"Warm Reset", ola::rdm::RESET_WARM},
    {"Cold Reset", ola::rdm::RESET_COLD}};

  for (unsigned int i = 0; i < sizeof(possible_values) / sizeof(values_s);
       ++i) {
    item->AddItem(possible_values[i].label, possible_values[i].state);
  }

  section.AddItem(item);
  section.SetSaveButton("Reset Device");
  RespondWithSection(response, section);

  return "";
}


/*
 * @brief Set the reset device.
 */
string RDMHTTPModule::SetResetDevice(const HTTPRequest *request,
                                     HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string reset_device_str = request->GetParameter(GENERIC_UINT_FIELD);
  uint8_t reset_device;
  ola::rdm::rdm_reset_device_mode reset_device_enum;
  if (!StringToInt(reset_device_str, &reset_device) ||
      !ola::rdm::UIntToResetDevice(reset_device, &reset_device_enum)) {
    return "Invalid reset device";
  }

  string error;
  m_rdm_api.SetResetDevice(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      reset_device_enum,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the DNS Hostname.
 */
string RDMHTTPModule::GetDnsHostname(HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string error;
  m_rdm_api.GetDnsHostname(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GetDnsHostnameHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the response to a DNS hostname call and build the response
 */
void RDMHTTPModule::GetDnsHostnameHandler(
    HTTPResponse *response,
    const ola::rdm::ResponseStatus &status,
    const string &label) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  section.AddItem(new StringItem("Hostname", label, GENERIC_STRING_FIELD));
  RespondWithSection(response, section);
}


/*
 * @brief Set the DNS hostname
 */
string RDMHTTPModule::SetDnsHostname(const HTTPRequest *request,
                                     HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string label = request->GetParameter(GENERIC_STRING_FIELD);
  string error;
  m_rdm_api.SetDnsHostname(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      label,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the request for the DNS domain name.
 */
string RDMHTTPModule::GetDnsDomainName(HTTPResponse *response,
                                       unsigned int universe_id,
                                       const UID &uid) {
  string error;
  m_rdm_api.GetDnsDomainName(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GetDnsDomainNameHandler,
                        response),
      &error);
  return error;
}


/**
 * @brief Handle the response to a DNS domain name call and build the response
 */
void RDMHTTPModule::GetDnsDomainNameHandler(
    HTTPResponse *response,
    const ola::rdm::ResponseStatus &status,
    const string &label) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  section.AddItem(new StringItem("Domain Name", label, GENERIC_STRING_FIELD));
  RespondWithSection(response, section);
}


/*
 * @brief Set the DNS domain name
 */
string RDMHTTPModule::SetDnsDomainName(const HTTPRequest *request,
                                       HTTPResponse *response,
                                       unsigned int universe_id,
                                       const UID &uid) {
  string label = request->GetParameter(GENERIC_STRING_FIELD);
  string error;
  m_rdm_api.SetDnsDomainName(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      label,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}

/**
 * @brief Handle the request for the dimmer curve section.
 */
string RDMHTTPModule::GetCurve(OLA_UNUSED const HTTPRequest *request,
                               HTTPResponse *response,
                               unsigned int universe_id,
                               const UID &uid,
                               bool include_descriptions) {
  string error;

  curve_info *info = new curve_info;
  info->universe_id = universe_id;
  info->uid = new UID(uid);
  info->include_descriptions = include_descriptions;
  info->active = 0;
  info->next = 1;
  info->total = 0;

  m_rdm_api.GetCurve(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GetCurveHandler,
                        response,
                        info),
      &error);
  return error;
}

/**
 * @brief Handle the response to a dimmer curve call and build the curve info
 * object
 */
void RDMHTTPModule::GetCurveHandler(
    HTTPResponse *response,
    curve_info *info,
    const ola::rdm::ResponseStatus &status,
    uint8_t active_curve,
    uint8_t curve_count) {
  if (CheckForRDMError(response, status)) {
    delete info->uid;
    delete info;
    return;
  }

  info->active = active_curve;
  info->total = curve_count;

  if (info->include_descriptions) {
    GetNextCurveDescription(response, info);
  } else {
    SendCurveResponse(response, info);
  }
}

/**
 * @brief Handle the response to a curve description call and add to curve
 * info object
 */
void RDMHTTPModule::GetNextCurveDescription(HTTPResponse *response,
                                            curve_info *info) {
  string error;
  while (info->next <= info->total) {
    bool r = m_rdm_api.GetCurveDescription(
        info->universe_id,
        *(info->uid),
        ola::rdm::ROOT_RDM_DEVICE,
        info->next,
        NewSingleCallback(this,
                          &RDMHTTPModule::GetCurveDescriptionHandler,
                          response,
                          info),
        &error);
    if (r) {
      return;
    }

    info->next++;
  }

  SendCurveResponse(response, info);
}

void RDMHTTPModule::GetCurveDescriptionHandler(
    HTTPResponse *response,
    curve_info *info,
    const ola::rdm::ResponseStatus &status,
    OLA_UNUSED uint8_t curve,
    const string &resp_description) {
  string description = "";

  if (CheckForRDMSuccess(status)) {
    description = resp_description;
  }

  info->curve_descriptions.push_back(description);

  if (info->next == info->total) {
    SendCurveResponse(response, info);
  } else {
    info->next++;
    GetNextCurveDescription(response, info);
  }
}

/**
 * @brief Build the curve http response
 */
void RDMHTTPModule::SendCurveResponse(HTTPResponse *response,
                                      curve_info *info) {
  JsonSection section;
  SelectItem *item = new SelectItem("Active Curve", GENERIC_UINT_FIELD);

  for (unsigned int i = 1; i <= info->total; i++) {
    if (i <= info->curve_descriptions.size() &&
        !info->curve_descriptions[i - 1].empty()) {
      ostringstream str;
      str << info->curve_descriptions[i - 1] << " ("
          << i << ")";
      item->AddItem(str.str(), i);
    } else {
      item->AddItem(IntToString(i), i);
    }
  }
  item->SetSelectedOffset(info->active - 1);

  section.AddItem(item);
  section.AddItem(new StringItem("Available Curves",
                                 IntToString(info->total)));
  RespondWithSection(response, section);
}

/**
 * @brief Set the dimmer curve
 */
string RDMHTTPModule::SetCurve(const HTTPRequest *request,
                                     HTTPResponse *response,
                                     unsigned int universe_id,
                                     const UID &uid) {
  string curve_str = request->GetParameter(GENERIC_UINT_FIELD);
  uint8_t curve;

  if (!StringToInt(curve_str, &curve)) {
    return "Invalid curve";
  }

  string error;
  m_rdm_api.SetCurve(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      curve,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}

/**
 * @brief Handle the request for Dimmer Info.
 */
string RDMHTTPModule::GetDimmerInfo(HTTPResponse *response,
                                    unsigned int universe_id,
                                    const UID &uid) {
  string error;
  m_rdm_api.GetDimmerInfo(
    universe_id,
    uid,
    ola::rdm::ROOT_RDM_DEVICE,
    NewSingleCallback(this,
                      &RDMHTTPModule::GetDimmerInfoHandler,
                      response),
    &error);
  return error;
}

/*
 * @brief Handle the response to a dimmer info call and build the response
 */
void RDMHTTPModule::GetDimmerInfoHandler(
    HTTPResponse *response,
    const ola::rdm::ResponseStatus &status,
    const ola::rdm::DimmerInfoDescriptor &info) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  section.AddItem(new UIntItem("Minimum Level Lower Limit",
                               info.min_level_lower_limit));
  section.AddItem(new UIntItem("Minimum Level Upper Limit",
                               info.min_level_upper_limit));
  section.AddItem(new UIntItem("Maximum Level Lower Limit",
                               info.max_level_lower_limit));
  section.AddItem(new UIntItem("Maximum Level Upper Limit",
                               info.max_level_upper_limit));
  section.AddItem(new UIntItem("# of Supported Curves", info.curves_supported));
  section.AddItem(new UIntItem("Levels Resolution", info.resolution));
  section.AddItem(new BoolItem("Split Levels Supported",
                               info.split_levels_supported));

  RespondWithSection(response, section);
}


/**
 * @brief Handle the request for Dimmer Minimum Levels.
 */
string RDMHTTPModule::GetDimmerMinimumLevels(HTTPResponse *response,
                                             unsigned int universe_id,
                                             const UID &uid) {
  string error;
  m_rdm_api.GetDimmerMinimumLevels(
    universe_id,
    uid,
    ola::rdm::ROOT_RDM_DEVICE,
    NewSingleCallback(this,
                      &RDMHTTPModule::GetDimmerMinimumLevelsHandler,
                      response),
    &error);
  return error;
}


/*
 * @brief Handle the response to a dimmer minimum levels and build the response
 */
void RDMHTTPModule::GetDimmerMinimumLevelsHandler(
    HTTPResponse *response,
    const ola::rdm::ResponseStatus &status,
    const ola::rdm::DimmerMinimumDescriptor &info) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  section.AddItem(new UIntItem("Minimum Level - Increasing",
                               info.min_level_increasing,
                               DIMMER_MINIMUM_INCREASING_FIELD));
  section.AddItem(new UIntItem("Minimum Level - Decreasing",
                               info.min_level_decreasing,
                               DIMMER_MINIMUM_DECREASING_FIELD));
  section.AddItem(new BoolItem("On Below Minimum", info.on_below_min,
                               GENERIC_BOOL_FIELD));

  RespondWithSection(response, section);
}


/**
 * @brief Set the dimmer minimum levels
 */
string RDMHTTPModule::SetDimmerMinimumLevels(const HTTPRequest *request,
                                             HTTPResponse *response,
                                             unsigned int universe_id,
                                             const UID &uid) {
  string raw_value = request->GetParameter(DIMMER_MINIMUM_INCREASING_FIELD);
  uint16_t min_increasing, min_decreasing;

  if (!StringToInt(raw_value, &min_increasing)) {
    return "Invalid minimum level - increasing";
  }

  raw_value = request->GetParameter(DIMMER_MINIMUM_DECREASING_FIELD);

  if (!StringToInt(raw_value, &min_decreasing)) {
    return "Invalid minimum level - decreasing";
  }

  raw_value = request->GetParameter(GENERIC_BOOL_FIELD);

  if (raw_value.empty()) {
    return "Invalid on below minimum value";
  }

  string error;
  m_rdm_api.SetDimmerMinimumLevels(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      min_increasing,
      min_decreasing,
      raw_value == "1",
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}

/**
 * @brief Handle the request for the dimmer maximum levels
 */
string RDMHTTPModule::GetDimmerMaximumLevel(HTTPResponse *response,
                                            unsigned int universe_id,
                                            const UID &uid) {
  string error;

  m_rdm_api.GetDimmerMaximumLevel(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHTTPModule::GetDimmerMaximumLevelHandler,
                        response),
      &error);
  return error;
}

/*
 * @brief Handle the response to a dimmer maximum call and build the response
 */
void RDMHTTPModule::GetDimmerMaximumLevelHandler(
    HTTPResponse *response,
    const ola::rdm::ResponseStatus &status,
    uint16_t maximum_level) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  section.AddItem(new UIntItem("Maximum Level",
                               maximum_level, GENERIC_UINT_FIELD));

  RespondWithSection(response, section);
}

/*
 * @brief Set the Dimmer maximum level
 */
string RDMHTTPModule::SetDimmerMaximumLevel(const HTTPRequest *request,
                                      HTTPResponse *response,
                                      unsigned int universe_id,
                                      const UID &uid) {
  string s_max_level = request->GetParameter(GENERIC_UINT_FIELD);
  uint16_t maximum_level;

  if (!StringToInt(s_max_level, &maximum_level)) {
    return "Invalid maximum level";
  }

  string error;
  m_rdm_api.SetDimmerMaximumLevel(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      maximum_level,
      NewSingleCallback(this,
                        &RDMHTTPModule::SetHandler,
                        response),
      &error);
  return error;
}

/**
 * @brief Check if the id URL param exists and is valid.
 */
bool RDMHTTPModule::CheckForInvalidId(const HTTPRequest *request,
                                      unsigned int *universe_id) {
  string uni_id = request->GetParameter(ID_KEY);
  if (!StringToInt(uni_id, universe_id)) {
    OLA_INFO << "Invalid universe id: " << uni_id;
    return false;
  }
  return true;
}


/**
 * @brief Check that the uid URL param exists and is valid.
 */
bool RDMHTTPModule::CheckForInvalidUid(const HTTPRequest *request,
                                       UID **uid) {
  string uid_string = request->GetParameter(UID_KEY);
  *uid = UID::FromString(uid_string);
  if (*uid == NULL) {
    OLA_INFO << "Invalid UID: " << uid_string;
    return false;
  }
  return true;
}


/**
 * @brief Get the sub device from the HTTP request, or return ROOT_DEVICE if it
 *   isn't valid.
 */
uint16_t RDMHTTPModule::SubDeviceOrRoot(const HTTPRequest *request) {
  string sub_device_str = request->GetParameter(SUB_DEVICE_FIELD);
  uint16_t sub_device;

  if (StringToInt(sub_device_str, &sub_device)) {
    return sub_device;
  }

  OLA_INFO << "Invalid sub device " << sub_device_str;
  return ola::rdm::ROOT_RDM_DEVICE;
}


/*
 * @brief Check the response to a Set RDM call and build the response.
 */
void RDMHTTPModule::SetHandler(
    HTTPResponse *response,
    const ola::rdm::ResponseStatus &status) {
  string error;
  CheckForRDMSuccessWithError(status, &error);
  RespondWithError(response, error);
}


/*
 * @brief Build a response to a RDM call that returns a uint32_t
 */
void RDMHTTPModule::GenericUIntHandler(HTTPResponse *response,
                                       string description,
                                       const ola::rdm::ResponseStatus &status,
                                       uint32_t value) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  section.AddItem(new UIntItem(description, value, GENERIC_UINT_FIELD));
  RespondWithSection(response, section);
}


/*
 * @brief Build a response to a RDM call that returns a bool
 */
void RDMHTTPModule::GenericUInt8BoolHandler(
    HTTPResponse *response,
    string description,
    const ola::rdm::ResponseStatus &status,
    uint8_t value) {
  GenericBoolHandler(response, description, status, value > 0);
}


/*
 * @brief Build a response to a RDM call that returns a bool
 */
void RDMHTTPModule::GenericBoolHandler(HTTPResponse *response,
                                       string description,
                                       const ola::rdm::ResponseStatus &status,
                                       bool value) {
  if (CheckForRDMError(response, status)) {
    return;
  }

  JsonSection section;
  section.AddItem(new BoolItem(description, value, GENERIC_BOOL_FIELD));
  RespondWithSection(response, section);
}

/**
 * @brief Check for an RDM error, and if it occurs, return a JSON response.
 * @return true if an error occurred.
 */
bool RDMHTTPModule::CheckForRDMError(HTTPResponse *response,
                                     const ola::rdm::ResponseStatus &status) {
  string error;
  if (!CheckForRDMSuccessWithError(status, &error)) {
    RespondWithError(response, error);
    return true;
  }
  return false;
}



int RDMHTTPModule::RespondWithError(HTTPResponse *response,
                                    const string &error) {
  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);

  JsonObject json;
  json.Add("error", error);
  int r = response->SendJson(json);
  delete response;
  return r;
}


/**
 * @brief Build & send a response from a JsonSection
 */
void RDMHTTPModule::RespondWithSection(HTTPResponse *response,
                                       const ola::web::JsonSection &section) {
  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->Append(section.AsString());
  response->Send();
  delete response;
}


/*
 * @brief Check the success of an RDM command
 * @returns true if this command was ok, false otherwise.
 */
bool RDMHTTPModule::CheckForRDMSuccess(
    const ola::rdm::ResponseStatus &status) {
  string error;
  if (!CheckForRDMSuccessWithError(status, &error)) {
    OLA_INFO << error;
    return false;
  }
  return true;
}


/*
 * @brief Check the success of an RDM command or return an error message.
 *
 * At the moment we're very strict in this method, some day this should be
 * relaxed to handle the corner cases.
 * @returns true if this command returns an ACK. false for any other condition.
 */
bool RDMHTTPModule::CheckForRDMSuccessWithError(
    const ola::rdm::ResponseStatus &status,
    string *error) {
  ostringstream str;
  if (!status.error.empty()) {
    str << "RDM command error: " << status.error;
    if (error) {
      *error = str.str();
    }
    return false;
  }

  // TODO(simon): One day we should handle broadcast responses, ack timers etc.
  if (status.response_code != ola::rdm::RDM_COMPLETED_OK) {
    if (error) {
      *error = ola::rdm::StatusCodeToString(status.response_code);
    }
  } else {
    switch (status.response_type) {
      case ola::rdm::RDM_ACK:
        return true;
      case ola::rdm::RDM_ACK_TIMER:
        str << "Got ACK Timer for " << status.AckTimer() << " ms";
        if (error) {
          *error = str.str();
        }
        break;
      case ola::rdm::RDM_NACK_REASON:
        str << "Request was NACKED with code: " <<
          ola::rdm::NackReasonToString(status.NackReason());
        OLA_INFO << str.str();
        if (error) {
          *error = str.str();
        }
    }
  }
  return false;
}


/*
 * @brief Handle the RDM discovery response
 * @param response the HTTPResponse that is associated with the request.
 * @param error an error string.
 */
void RDMHTTPModule::HandleBoolResponse(HTTPResponse *response,
                                       const string &error) {
  if (!error.empty()) {
    m_server->ServeError(response, error);
    return;
  }
  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->Append("ok");
  response->Send();
  delete response;
}


/**
 * @brief Add a section to the supported section list
 */
void RDMHTTPModule::AddSection(vector<section_info> *sections,
                               const string &section_id,
                               const string &section_name,
                               const string &hint) {
  section_info info = {section_id, section_name, hint};
  sections->push_back(info);
}
}  // namespace ola
