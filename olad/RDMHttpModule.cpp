/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * RDMHttpModule.cpp
 * This module acts as the http -> olad gateway for RDM commands.
 * Copyright (C) 2010 Simon Newton
 */

#include <algorithm>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/OlaCallbackClient.h"
#include "ola/StringUtils.h"
#include "ola/rdm/RDMHelper.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "olad/OlaServer.h"
#include "olad/RDMHttpModule.h"


namespace ola {

using ola::rdm::UID;
using std::endl;
using std::pair;
using std::string;
using std::stringstream;
using std::vector;


const char RDMHttpModule::BACKEND_DISCONNECTED_ERROR[] =
    "Failed to send request, client isn't connected";
const char RDMHttpModule::HINT_KEY[] = "hint";
const char RDMHttpModule::ID_KEY[] = "id";
const char RDMHttpModule::SECTION_KEY[] = "section";
const char RDMHttpModule::UID_KEY[] = "uid";


/**
 * Create a new OLA HTTP server
 * @param export_map the ExportMap to display when /debug is called
 * @param client_socket A ConnectedSocket which is used to communicate with the
 *   server.
 * @param
 */
RDMHttpModule::RDMHttpModule(HttpServer *http_server,
                             OlaCallbackClient *client)
    : HttpModule(http_server, client),
      m_server(http_server),
      m_client(client),
      m_rdm_api(m_client) {

  m_server->RegisterHandler(
      "/rdm/run_discovery",
      NewCallback(this, &RDMHttpModule::RunRDMDiscovery));
  m_server->RegisterHandler(
      "/json/rdm/uids",
      NewCallback(this, &RDMHttpModule::JsonUIDs));
  m_server->RegisterHandler(
      "/json/rdm/supported_pids",
      NewCallback(this, &RDMHttpModule::JsonSupportedPIDs));
  m_server->RegisterHandler(
      "/json/rdm/supported_sections",
      NewCallback(this, &RDMHttpModule::JsonSupportedSections));
  m_server->RegisterHandler(
      "/json/rdm/section_info",
      NewCallback(this, &RDMHttpModule::JsonSectionInfo));
}


/*
 * Teardown
 */
RDMHttpModule::~RDMHttpModule() {
  map<unsigned int, uid_resolution_state*>::iterator uid_iter;
  for (uid_iter = m_universe_uids.begin(); uid_iter != m_universe_uids.end();
       uid_iter++) {
    delete uid_iter->second;
  }
  m_universe_uids.clear();
}


/**
 * Run RDM discovery for a universe
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int RDMHttpModule::RunRDMDiscovery(const HttpRequest *request,
                                   HttpResponse *response) {
  unsigned int universe_id;
  if (!CheckForInvalidId(request, &universe_id))
    return m_server->ServeNotFound(response);

  bool ok = m_client->ForceDiscovery(
      universe_id,
      NewSingleCallback(this,
                        &RDMHttpModule::HandleBoolResponse,
                        response));

  if (!ok)
    return m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
}


/**
 * Return the list of uids for this universe as json
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int RDMHttpModule::JsonUIDs(const HttpRequest *request,
                            HttpResponse *response) {
  unsigned int universe_id;
  if (!CheckForInvalidId(request, &universe_id))
    return m_server->ServeNotFound(response);

  bool ok = m_client->FetchUIDList(
      universe_id,
      NewSingleCallback(this,
                        &RDMHttpModule::HandleUIDList,
                        response,
                        universe_id));

  if (!ok)
    return m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
}


/**
 * Return a list of pids supported by this device. This isn't used by the UI
 * but it's useful for debugging.
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int RDMHttpModule::JsonSupportedPIDs(const HttpRequest *request,
                                     HttpResponse *response) {
  unsigned int universe_id;
  if (!CheckForInvalidId(request, &universe_id))
    return m_server->ServeNotFound(response);

  UID *uid = NULL;
  if (!CheckForInvalidUid(request, &uid))
    return m_server->ServeNotFound(response);

  string error;
  bool ok = m_rdm_api.GetSupportedParameters(
      universe_id,
      *uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHttpModule::SupportedParamsHandler,
                        response),
      &error);
  delete uid;

  if (!ok)
    return m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
}


/**
 * Return a list of sections to display in the RDM control panel.
 * We use the response from SUPPORTED_PARAMS and DEVICE_INFO to decide which
 * pids exist.
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int RDMHttpModule::JsonSupportedSections(const HttpRequest *request,
                                         HttpResponse *response) {
  unsigned int universe_id;
  if (!CheckForInvalidId(request, &universe_id))
    return m_server->ServeNotFound(response);

  UID *uid = NULL;
  if (!CheckForInvalidUid(request, &uid))
    return m_server->ServeNotFound(response);

  string error;
  bool ok = m_rdm_api.GetSupportedParameters(
      universe_id,
      *uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHttpModule::SupportedSectionsHandler,
                        response,
                        universe_id,
                        *uid),
      &error);
  delete uid;

  if (!ok)
    return m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
}


/**
 * Get the information required to render a section in the RDM controller panel
 */
int RDMHttpModule::JsonSectionInfo(const HttpRequest *request,
                                   HttpResponse *response) {
  unsigned int universe_id;
  if (!CheckForInvalidId(request, &universe_id))
    return m_server->ServeNotFound(response);

  UID *uid = NULL;
  if (!CheckForInvalidUid(request, &uid))
    return m_server->ServeNotFound(response);

  string section_id = request->GetParameter(SECTION_KEY);
  string error;
  if (section_id == "device_info") {
    error = ProcessDeviceInfo(request, response, universe_id, *uid);
  } else if (section_id == "product_detail") {
    error = ProcessDetailIds(request, response, universe_id, *uid);
  } else if (section_id == "manufacturer_label") {
    error = ProcessManufacturerLabel(request, response, universe_id, *uid);
  } else if (section_id == "device_label") {
    error = ProcessDeviceLabel(request, response, universe_id, *uid);
  } else if (section_id == "dmx_address") {
    error = ProcessStartAddress(request, response, universe_id, *uid);
  } else {
    OLA_INFO << "Missing or unknown section id: " << section_id;
    return m_server->ServeNotFound(response);
  }

  if (!error.empty())
    return m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR + error);
  return MHD_YES;
}


/**
 * This is called from the main http server whenever a new list of active
 * universes is received. It's used to prune the uid map so we don't bother
 * trying to resolve uids for universes that no longer exist.
 */
void RDMHttpModule::PruneUniverseList(const vector<OlaUniverse> &universes) {
  map<unsigned int, uid_resolution_state*>::iterator uid_iter;
  for (uid_iter = m_universe_uids.begin(); uid_iter != m_universe_uids.end();
       uid_iter++) {
    uid_iter->second->active = false;
  }

  vector<OlaUniverse>::const_iterator iter;
  for (iter = universes.begin(); iter != universes.end(); ++iter) {
    uid_iter = m_universe_uids.find(iter->Id());
    if (uid_iter != m_universe_uids.end())
      uid_iter->second->active = true;
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
 * Handle the UID list response.
 * @param response the HttpResponse that is associated with the request.
 * @param uids the UIDs for this response.
 * @param error an error string.
 */
void RDMHttpModule::HandleUIDList(HttpResponse *response,
                                  unsigned int universe_id,
                                  const ola::rdm::UIDSet &uids,
                                  const string &error) {
  if (!error.empty()) {
    m_server->ServeError(response, error);
    return;
  }
  ola::rdm::UIDSet::Iterator iter = uids.Begin();
  uid_resolution_state *uid_state = GetUniverseUidsOrCreate(universe_id);

  // mark all uids as inactive so we can remove the unused ones at the end
  map<UID, resolved_uid>::iterator uid_iter;
  for (uid_iter = uid_state->resolved_uids.begin();
       uid_iter != uid_state->resolved_uids.end(); ++uid_iter)
    uid_iter->second.active = false;

  stringstream str;
  str << "{" << endl;
  str << "  \"universe\": " << universe_id << "," << endl;
  str << "  \"uids\": [" << endl;

  for (; iter != uids.End(); ++iter) {
    uid_iter = uid_state->resolved_uids.find(*iter);

    string manufacturer = "";
    string device = "";

    if (uid_iter == uid_state->resolved_uids.end()) {
      // schedule resolution
      uid_state->pending_uids.push(
          std::pair<UID, uid_resolve_action>(*iter, RESOLVE_MANUFACTURER));
      uid_state->pending_uids.push(
          std::pair<UID, uid_resolve_action>(*iter, RESOLVE_DEVICE));
      resolved_uid uid_descriptor = {"", "", true};
      uid_state->resolved_uids[*iter] = uid_descriptor;
      OLA_DEBUG << "Adding UID " << uid_iter->first << " to resolution queue";
    } else {
      manufacturer = uid_iter->second.manufacturer;
      device = uid_iter->second.device;
      uid_iter->second.active = true;
    }
    str << "    {" << endl;
    str << "       \"manufacturer_id\": " << iter->ManufacturerId() << ","
      << endl;
    str << "       \"device_id\": " << iter->DeviceId() << "," << endl;
    str << "       \"device\": \"" << EscapeString(device) << "\"," << endl;
    str << "       \"manufacturer\": \"" << EscapeString(manufacturer) <<
      "\"," << endl;
    str << "    }," << endl;
  }

  str << "  ]" << endl;
  str << "}";

  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  response->Send();
  delete response;

  // remove any old uids
  for (uid_iter = uid_state->resolved_uids.begin();
       uid_iter != uid_state->resolved_uids.end();) {
    if (!uid_iter->second.active) {
      OLA_DEBUG << "Removed UID " << uid_iter->first;
      uid_state->resolved_uids.erase(uid_iter++);
    } else {
      ++uid_iter;
    }
  }

  if (!uid_state->uid_resolution_running)
    ResolveNextUID(universe_id);
}


/*
 * Send the RDM command needed to resolve the next uid in the queue
 * @param universe_id the universe id to resolve the next UID for.
 */
void RDMHttpModule::ResolveNextUID(unsigned int universe_id) {
  bool sent_request = false;
  string error;
  uid_resolution_state *uid_state = GetUniverseUids(universe_id);

  if (!uid_state)
    return;

  while (!sent_request) {
    if (!uid_state->pending_uids.size()) {
      uid_state->uid_resolution_running = false;
      return;
    }
    uid_state->uid_resolution_running = true;

    pair<UID, uid_resolve_action> &uid_action_pair =
      uid_state->pending_uids.front();
    if (uid_action_pair.second == RESOLVE_MANUFACTURER) {
      OLA_DEBUG << "sending manufacturer request for " << uid_action_pair.first;
      sent_request = m_rdm_api.GetManufacturerLabel(
          universe_id,
          uid_action_pair.first,
          ola::rdm::ROOT_RDM_DEVICE,
          NewSingleCallback(this,
                            &RDMHttpModule::UpdateUIDManufacturerLabel,
                            universe_id,
                            uid_action_pair.first),
          &error);
      OLA_DEBUG << "return code was " << sent_request;
      uid_state->pending_uids.pop();
    } else if (uid_action_pair.second == RESOLVE_DEVICE) {
      OLA_INFO << "sending device request for " << uid_action_pair.first;
      sent_request = m_rdm_api.GetDeviceLabel(
          universe_id,
          uid_action_pair.first,
          ola::rdm::ROOT_RDM_DEVICE,
          NewSingleCallback(this,
                            &RDMHttpModule::UpdateUIDDeviceLabel,
                            universe_id,
                            uid_action_pair.first),
          &error);
      uid_state->pending_uids.pop();
      OLA_DEBUG << "return code was " << sent_request;
    } else {
      OLA_WARN << "Unknown UID resolve action " <<
        static_cast<int>(uid_action_pair.second);
    }
  }
}

/*
 * Handle the manufacturer label response.
 */
void RDMHttpModule::UpdateUIDManufacturerLabel(
    unsigned int universe,
    UID uid,
    const ola::rdm::ResponseStatus &status,
    const string &manufacturer_label) {
  uid_resolution_state *uid_state = GetUniverseUids(universe);

  if (!uid_state)
    return;

  if (CheckForRDMSuccess(status)) {
    map<UID, resolved_uid>::iterator uid_iter;
    uid_iter = uid_state->resolved_uids.find(uid);
    if (uid_iter != uid_state->resolved_uids.end())
      uid_iter->second.manufacturer = manufacturer_label;
  }
  ResolveNextUID(universe);
}


/*
 * Handle the device label response.
 */
void RDMHttpModule::UpdateUIDDeviceLabel(
    unsigned int universe,
    UID uid,
    const ola::rdm::ResponseStatus &status,
    const string &device_label) {
  uid_resolution_state *uid_state = GetUniverseUids(universe);

  if (!uid_state)
    return;

  if (CheckForRDMSuccess(status)) {
    map<UID, resolved_uid>::iterator uid_iter;
    uid_iter = uid_state->resolved_uids.find(uid);
    if (uid_iter != uid_state->resolved_uids.end())
      uid_iter->second.device = device_label;
  }
  ResolveNextUID(universe);
}


/*
 * Get the UID resolution state for a particular universe
 * @param universe the id of the universe to get the state for
 */
RDMHttpModule::uid_resolution_state *RDMHttpModule::GetUniverseUids(
    unsigned int universe) {
  map<unsigned int, uid_resolution_state*>::iterator iter =
    m_universe_uids.find(universe);
  return iter == m_universe_uids.end() ? NULL : iter->second;
}


/*
 * Get the UID resolution state for a particular universe or create one if it
 * doesn't exist.
 * @param universe the id of the universe to get the state for
 */
RDMHttpModule::uid_resolution_state *RDMHttpModule::GetUniverseUidsOrCreate(
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


/*
 * Handle the response from a supported params request
 */
void RDMHttpModule::SupportedParamsHandler(
    HttpResponse *response,
    const ola::rdm::ResponseStatus &status,
    const vector<uint16_t> &pids) {
  stringstream str;
  if (CheckForRDMSuccess(status)) {
    vector<uint16_t>::const_iterator iter = pids.begin();

    str << "{" << endl;
    str << "  \"pids\": [" << endl;

    for (; iter != pids.end(); ++iter) {
      str << "    0x" << std::hex << *iter << ",\n";
    }

    str << "  ]" << endl;
    str << "}";
  }

  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  response->Send();
  delete response;
}


/**
 * Takes the supported pids for a device and come up with the list of sections
 * to display in the RDM panel
 */
void RDMHttpModule::SupportedSectionsHandler(
    HttpResponse *response,
    unsigned int universe_id,
    UID uid,
    const ola::rdm::ResponseStatus &status,
    const vector<uint16_t> &pid_list) {
  string error;

  // nacks here are ok if the device doesn't support SUPPORTED_PARAMS
  if (!CheckForRDMSuccess(status) &&
      status.ResponseType() != ola::rdm::ResponseStatus::REQUEST_NACKED) {
    m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR + error);
    return;
  }

  m_rdm_api.GetDeviceInfo(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHttpModule::SupportedSectionsDeviceInfoHandler,
                        response,
                        pid_list),
      &error);
  if (!error.empty())
    m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR + error);
}


/**
 * Handle the second part of the supported sections request.
 */
void RDMHttpModule::SupportedSectionsDeviceInfoHandler(
    HttpResponse *response,
    const vector<uint16_t> pid_list,
    const ola::rdm::ResponseStatus &status,
    const ola::rdm::DeviceDescriptor &device) {
  vector<section_info> sections;
  std::set<uint16_t> pids;
  copy(pid_list.begin(), pid_list.end(), inserter(pids, pids.end()));

  // PID_DEVICE_INFO is required so we always add it
  string hint;
  if (pids.find(ola::rdm::PID_DEVICE_MODEL_DESCRIPTION) != pids.end())
      hint.push_back('m');  // m is for device model
  AddSection(&sections, "device_info", "Device Info", hint);

  AddSection(&sections, "identify", "Identify Mode", hint);

  bool dmx_address_added = false;
  vector<uint16_t>::const_iterator iter = pid_list.begin();
  for (; iter != pid_list.end(); ++iter) {
    switch (*iter) {
      case ola::rdm::PID_MANUFACTURER_LABEL:
        AddSection(&sections, "manufacturer_label", "Manufacturer Label", "");
        break;
      case ola::rdm::PID_DEVICE_LABEL:
        AddSection(&sections, "device_label", "Device Label", "");
        break;
      case ola::rdm::PID_DMX_START_ADDRESS:
        AddSection(&sections, "dmx_address", "DMX Start Address", "");
        dmx_address_added = true;
        break;
      case ola::rdm::PID_PRODUCT_DETAIL_ID_LIST:
        AddSection(&sections, "product_detail", "Product Details", "");
        break;
    }
  }

  if (CheckForRDMSuccess(status)) {
    if (device.dmx_footprint && !dmx_address_added)
      AddSection(&sections, "dmx_address", "DMX Start Address", "");
    if (device.sensor_count &&
        pids.find(ola::rdm::PID_SENSOR_DEFINITION) != pids.end() &&
        pids.find(ola::rdm::PID_SENSOR_VALUE) != pids.end()) {
      // sensors count from 1
      for (unsigned int i = 1; i <= device.sensor_count; ++i) {
        stringstream heading, hint;
        hint << i;
        heading << "Sensor " << i;
        AddSection(&sections, "sensor", heading.str(), hint.str());
      }
    }
  }

  sort(sections.begin(), sections.end(), lt_section_info());

  vector<section_info>::const_iterator section_iter;
  stringstream str;
  str << "[" << endl;
  for (section_iter = sections.begin(); section_iter != sections.end();
       ++section_iter) {
    str << "  {" << endl;
    str << "    \"id\": \"" << section_iter->id << "\"," << endl;
    str << "    \"name\": \"" << section_iter->name << "\"," << endl;
    str << "    \"hint\": \"" << section_iter->hint << "\"," << endl;
    str << "  }," << endl;
  }
  str << "]" << endl;
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  response->Send();
  delete response;
}


/*
 * Handle the request for the device info section.
 */
string RDMHttpModule::ProcessDeviceInfo(const HttpRequest *request,
                                        HttpResponse *response,
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
                      &RDMHttpModule::GetSoftwareVersionHandler,
                      response,
                      dev_info),
    &error);
  return error;
}


/**
 * Handle the response to a software version call.
 */
void RDMHttpModule::GetSoftwareVersionHandler(
    HttpResponse *response,
    device_info dev_info,
    const ola::rdm::ResponseStatus &status,
    const string &software_version) {
  string error;

  if (CheckForRDMSuccess(status))
    dev_info.software_version = software_version;

  if (dev_info.hint.find('m') != string::npos) {
    m_rdm_api.GetDeviceModelDescription(
        dev_info.universe_id,
        dev_info.uid,
        ola::rdm::ROOT_RDM_DEVICE,
        NewSingleCallback(this,
                          &RDMHttpModule::GetDeviceModelHandler,
                          response,
                          dev_info),
        &error);
  } else {
    m_rdm_api.GetDeviceInfo(
        dev_info.universe_id,
        dev_info.uid,
        ola::rdm::ROOT_RDM_DEVICE,
        NewSingleCallback(this,
                          &RDMHttpModule::GetDeviceInfoHandler,
                          response,
                          dev_info),
        &error);
  }

  if (!error.empty())
    m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR + error);
}


/**
 * Handle the response to a device model call.
 */
void RDMHttpModule::GetDeviceModelHandler(
    HttpResponse *response,
    device_info dev_info,
    const ola::rdm::ResponseStatus &status,
    const string &device_model) {
  string error;

  if (CheckForRDMSuccess(status))
    dev_info.device_model = device_model;

  m_rdm_api.GetDeviceInfo(
      dev_info.universe_id,
      dev_info.uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHttpModule::GetDeviceInfoHandler,
                        response,
                        dev_info),
      &error);

  if (!error.empty())
    m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR + error);
}


/**
 * Handle the response to a device info call and build the response
 */
void RDMHttpModule::GetDeviceInfoHandler(
    HttpResponse *response,
    device_info dev_info,
    const ola::rdm::ResponseStatus &status,
    const ola::rdm::DeviceDescriptor &device) {
  stringstream str;
  str << "[" << endl;
  if (CheckForRDMSuccess(status)) {
    stringstream stream;
    stream << static_cast<int>(device.protocol_version_high) << "."
      << static_cast<int>(device.protocol_version_low);
    AddStringVariable(&str, "Protocol Version", stream.str());

    stream.str("");
    if (dev_info.device_model.empty())
      stream << device.device_model;
    else
      stream << dev_info.device_model << " (" << device.device_model << ")";
    AddStringVariable(&str, "Device Model", stream.str());

    AddStringVariable(
        &str,
        "Product Category",
        ola::rdm::ProductCategoryToString(device.product_category));
    stream.str("");
    if (dev_info.software_version.empty())
      stream << device.software_version;
    else
      stream << dev_info.software_version << " (" << device.software_version
        << ")";
    AddStringVariable(&str, "Software Version", stream.str());
    AddIntVariable(&str, "DMX Footprint", device.dmx_footprint);

    stream.str("");
    stream << static_cast<int>(device.current_personality) << " of " <<
      static_cast<int>(device.personaility_count);
    AddStringVariable(&str, "Personality", stream.str());

    AddIntVariable(&str, "Sub Devices", device.sub_device_count);
    AddIntVariable(&str, "Sensors", device.sensor_count);
  }
  str << "]";
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  response->Send();
  delete response;
}


/*
 * Handle the request for the product details ids.
 */
string RDMHttpModule::ProcessDetailIds(const HttpRequest *request,
                                       HttpResponse *response,
                                       unsigned int universe_id,
                                       const UID &uid) {
  string error;
  m_rdm_api.GetProductDetailIdList(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHttpModule::GetProductIdsHandler,
                        response),
      &error);
  return error;
  (void) request;
}


/**
 * Handle the response to a product detail ids call and build the response.
 */
void RDMHttpModule::GetProductIdsHandler(
    HttpResponse *response,
    const ola::rdm::ResponseStatus &status,
    const vector<uint16_t> &ids) {
  bool first = true;
  stringstream str, product_ids;
  str << "[" << endl;
  if (CheckForRDMSuccess(status)) {
    vector<uint16_t>::const_iterator iter = ids.begin();
    for (; iter != ids.end(); ++iter) {
      string product_id = ola::rdm::ProductDetailToString(*iter);
      if (product_id.empty())
        continue;

      if (first)
        first = false;
      else
        product_ids << ", ";
      product_ids << product_id;
    }
    AddStringVariable(&str, "Product Detail IDs", product_ids.str());
  }
  str << "]";
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  response->Send();
  delete response;
}


/**
 * Handle the request for the Manufacturer label.
 */
string RDMHttpModule::ProcessManufacturerLabel(const HttpRequest *request,
                                               HttpResponse *response,
                                               unsigned int universe_id,
                                               const UID &uid) {
  string error;
  m_rdm_api.GetManufacturerLabel(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHttpModule::GetManufacturerLabelHandler,
                        response),
      &error);
  return error;
  (void) request;
}


/**
 * Handle the response to a manufacturer label call and build the response
 */
void RDMHttpModule::GetManufacturerLabelHandler(
    HttpResponse *response,
    const ola::rdm::ResponseStatus &status,
    const string &label) {
  stringstream str;
  str << "[" << endl;
  if (CheckForRDMSuccess(status))
    AddStringVariable(&str, "Manufacturer Label", label, true);
  str << "]";
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  response->Send();
  delete response;
}


/**
 * Handle the request for the Device label.
 */
string RDMHttpModule::ProcessDeviceLabel(const HttpRequest *request,
                                         HttpResponse *response,
                                         unsigned int universe_id,
                                         const UID &uid) {
  string error;
  m_rdm_api.GetDeviceLabel(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHttpModule::GetDeviceLabelHandler,
                        response),
      &error);
  return error;
  (void) request;
}


/**
 * Handle the response to a device label call and build the response
 */
void RDMHttpModule::GetDeviceLabelHandler(
    HttpResponse *response,
    const ola::rdm::ResponseStatus &status,
    const string &label) {
  stringstream str;
  str << "[" << endl;
  if (CheckForRDMSuccess(status))
    AddStringVariable(&str, "Device Label", label, true);
  str << "]";
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  response->Send();
  delete response;
}


/**
 * Handle the request for the start address section.
 */
string RDMHttpModule::ProcessStartAddress(const HttpRequest *request,
                                          HttpResponse *response,
                                          unsigned int universe_id,
                                          const UID &uid) {
  string error;
  m_rdm_api.GetDMXAddress(
      universe_id,
      uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHttpModule::GetStartAddressHandler,
                        response),
      &error);
  return error;
  (void) request;
}


/**
 * Handle the response to a dmx start address call and build the response
 */
void RDMHttpModule::GetStartAddressHandler(
    HttpResponse *response,
    const ola::rdm::ResponseStatus &status,
    uint16_t address) {
  stringstream str;
  str << "[" << endl;
  if (CheckForRDMSuccess(status))
    AddIntVariable(&str, "DMX Start Address", address, true);
  str << "]";
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  response->Send();
  delete response;
}


/**
 * Check if the id url param exists and is valid.
 */
bool RDMHttpModule::CheckForInvalidId(const HttpRequest *request,
                                      unsigned int *universe_id) {
  string uni_id = request->GetParameter(ID_KEY);
  if (!StringToUInt(uni_id, universe_id)) {
    OLA_INFO << "Invalid universe id: " << uni_id;
    return false;
  }
  return true;
}


/**
 * Check that the uid url param exists and is valid.
 */
bool RDMHttpModule::CheckForInvalidUid(const HttpRequest *request,
                                       UID **uid) {
  string uid_string = request->GetParameter(UID_KEY);
  *uid = UID::FromString(uid_string);
  if (uid == NULL) {
    OLA_INFO << "Invalid uid: " << uid_string;
    return false;
  }
  return true;
}


/*
 * Check the success of an RDM command
 * @returns true if this command was ok, false otherwise.
 */
bool RDMHttpModule::CheckForRDMSuccess(
    const ola::rdm::ResponseStatus &status) {
  switch (status.ResponseType()) {
    case ola::rdm::ResponseStatus::TRANSPORT_ERROR:
      OLA_INFO << "RDM command error: " << status.Error();
      return false;
    case ola::rdm::ResponseStatus::BROADCAST_REQUEST:
      return false;
    case ola::rdm::ResponseStatus::REQUEST_NACKED:
      OLA_INFO << "Request was NACKED with code: " <<
        ola::rdm::NackReasonToString(status.NackReason());
      return false;
    case ola::rdm::ResponseStatus::MALFORMED_RESPONSE:
      OLA_INFO << "Malformed RDM response " << status.Error();
      return false;
    case ola::rdm::ResponseStatus::VALID_RESPONSE:
      return true;
    default:
      OLA_INFO << "Unknown response status " <<
        static_cast<int>(status.ResponseType());
      return false;
  }
}


/*
 * Handle the RDM discovery response
 * @param response the HttpResponse that is associated with the request.
 * @param error an error string.
 */
void RDMHttpModule::HandleBoolResponse(HttpResponse *response,
                                       const string &error) {
  if (!error.empty()) {
    m_server->ServeError(response, error);
    return;
  }
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append("ok");
  response->Send();
  delete response;
}


/**
 * Add a section to the supported section list
 */
void RDMHttpModule::AddSection(vector<section_info> *sections,
                               const string &section_id,
                               const string &section_name,
                               const string &hint) {
  section_info info = {section_id, section_name, hint};
  sections->push_back(info);
}


/**
 * Add a int variable to the output
 */
void RDMHttpModule::AddIntVariable(stringstream *str,
                                   const string &name,
                                   unsigned int value,
                                   bool editable) {
  *str << "  {" << endl;
  *str << "  \"name\": \"" << name << "\"," << endl;
  *str << "  \"type\": \"int\"," << endl;
  *str << "  \"editable\": " << editable << "," << endl;
  *str << "  \"value\": " << value << endl;
  *str << "  }," << endl;
}


/**
 * Add a string variable to the output
 */
void RDMHttpModule::AddStringVariable(stringstream *str,
                                      const string &name,
                                      const string &value,
                                      bool editable) {
  *str << "  {" << endl;
  *str << "  \"name\": \"" << name << "\"," << endl;
  *str << "  \"type\": \"string\"," << endl;
  *str << "  \"editable\": " << editable << "," << endl;
  *str << "  \"value\": \"" << value << "\"," << endl;
  *str << "  }," << endl;
}
}  // ola
