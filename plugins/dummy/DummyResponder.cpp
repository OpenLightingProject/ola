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
 * DummyResponder.cpp
 * The Dummy Responder for ola
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/dummy/DummyResponder.h"

namespace ola {
namespace plugin {
namespace dummy {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using ola::rdm::GetResponseFromData;
using ola::rdm::NackWithReason;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using std::string;
using std::vector;


const DummyResponder::personality_info DummyResponder::PERSONALITIES[] = {
  {5, "Personality 1"},
  {10, "Personality 2"},
  {20, "Personality 3"},
};

const unsigned int DummyResponder::PERSONALITY_COUNT = (
  sizeof(DummyResponder::PERSONALITIES) /
  sizeof(DummyResponder::personality_info));


/*
 * Handle an RDM Request
 */
void DummyResponder::SendRDMRequest(const ola::rdm::RDMRequest *request,
                                    ola::rdm::RDMCallback *callback) {
  if (request->DestinationUID() != m_uid &&
      (!request->DestinationUID().IsBroadcast())) {
    vector<string> packets;
    OLA_WARN << "Dummy responder received request for the wrong UID, " <<
      "expected " << m_uid << ", got " << request->DestinationUID();
    delete request;
    callback->Run(ola::rdm::RDM_TIMEOUT, NULL, packets);
    return;
  }

  switch (request->ParamId()) {
    case ola::rdm::PID_SUPPORTED_PARAMETERS:
      HandleSupportedParams(request, callback);
      break;
    case ola::rdm::PID_DEVICE_INFO:
      HandleDeviceInfo(request, callback);
      break;
    case ola::rdm::PID_PRODUCT_DETAIL_ID_LIST:
      HandleProductDetailList(request, callback);
      break;
    case ola::rdm::PID_MANUFACTURER_LABEL:
      HandleStringResponse(request, callback, "Open Lighting");
      break;
    case ola::rdm::PID_DEVICE_LABEL:
      HandleStringResponse(request, callback, "Dummy RDM Device");
      break;
    case ola::rdm::PID_DEVICE_MODEL_DESCRIPTION:
      HandleStringResponse(request, callback, "Dummy Model");
      break;
    case ola::rdm::PID_SOFTWARE_VERSION_LABEL:
      HandleStringResponse(request, callback, "Dummy Software Version");
      break;
    case ola::rdm::PID_DMX_PERSONALITY:
      HandlePersonality(request, callback);
      break;
    case ola::rdm::PID_DMX_PERSONALITY_DESCRIPTION:
      HandlePersonalityDescription(request, callback);
      break;
    case ola::rdm::PID_DMX_START_ADDRESS:
      HandleDmxStartAddress(request, callback);
      break;
    case ola::rdm::PID_IDENTIFY_DEVICE:
      HandleIdentifyDevice(request, callback);
      break;
    default:
      HandleUnknownPacket(request, callback);
  }
}


void DummyResponder::HandleUnknownPacket(const RDMRequest *request,
                                         ola::rdm::RDMCallback *callback) {
  if (callback) {
    if (request->DestinationUID().IsBroadcast()) {
      // no responses for broadcasts
      vector<string> packets;
      callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
    } else if (callback) {
      RDMResponse *response = NackWithReason(request,
                                             ola::rdm::NR_UNKNOWN_PID);
      RunRDMCallback(callback, response);
    }
  }
  delete request;
}


void DummyResponder::HandleSupportedParams(const RDMRequest *request,
                                           ola::rdm::RDMCallback *callback) {
  if (!CheckForBroadcastSubdeviceOrData(request, callback))
    return;

  uint16_t supported_params[] = {
    ola::rdm::PID_DEVICE_LABEL,
    ola::rdm::PID_DEVICE_MODEL_DESCRIPTION,
    ola::rdm::PID_DMX_PERSONALITY,
    ola::rdm::PID_DMX_PERSONALITY_DESCRIPTION,
    ola::rdm::PID_MANUFACTURER_LABEL,
    ola::rdm::PID_PRODUCT_DETAIL_ID_LIST
  };

  for (unsigned int i = 0; i < sizeof(supported_params) / 2; i++)
    supported_params[i] = HostToNetwork(supported_params[i]);

  RDMResponse *response = GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(supported_params),
      sizeof(supported_params));

  RunRDMCallback(callback, response);
  delete request;
}


void DummyResponder::HandleDeviceInfo(const RDMRequest *request,
                                      ola::rdm::RDMCallback *callback) {
  if (!CheckForBroadcastSubdeviceOrData(request, callback))
    return;

  struct device_info_s {
    uint16_t rdm_version;
    uint16_t model;
    uint16_t product_category;
    uint32_t software_version;
    uint16_t dmx_footprint;
    uint8_t current_personality;
    uint8_t personality_count;
    uint16_t dmx_start_address;
    uint16_t sub_device_count;
    uint8_t sensor_count;
  } __attribute__((packed));

  struct device_info_s device_info;
  device_info.rdm_version = HostToNetwork(static_cast<uint16_t>(0x100));
  device_info.model = HostToNetwork(static_cast<uint16_t>(1));
  device_info.product_category = HostToNetwork(
      static_cast<uint16_t>(ola::rdm::PRODUCT_CATEGORY_OTHER));
  device_info.software_version = HostToNetwork(static_cast<uint32_t>(1));
  device_info.dmx_footprint =
    HostToNetwork(PERSONALITIES[m_personality].footprint);
  device_info.current_personality = m_personality + 1;
  device_info.personality_count = PERSONALITY_COUNT;
  device_info.dmx_start_address = HostToNetwork(m_start_address);
  device_info.sub_device_count = 0;
  device_info.sensor_count = 0;
  RDMResponse *response = GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&device_info),
      sizeof(device_info));
  RunRDMCallback(callback, response);
  delete request;
}


/**
 * Handle a request for PID_PRODUCT_DETAIL_ID_LIST
 */
void DummyResponder::HandleProductDetailList(const RDMRequest *request,
                                             ola::rdm::RDMCallback *callback) {
  if (!CheckForBroadcastSubdeviceOrData(request, callback))
    return;

  uint16_t product_details[] = {
    ola::rdm::PRODUCT_DETAIL_TEST,
    ola::rdm::PRODUCT_DETAIL_OTHER
  };

  for (unsigned int i = 0; i < sizeof(product_details) / 2; i++)
    product_details[i] = HostToNetwork(product_details[i]);

  RDMResponse *response = GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&product_details),
      sizeof(product_details));
  RunRDMCallback(callback, response);
  delete request;
}


/*
 * Handle a request that returns a string
 */
void DummyResponder::HandleStringResponse(const ola::rdm::RDMRequest *request,
                                          ola::rdm::RDMCallback *callback,
                                          const string &value) {
  if (!CheckForBroadcastSubdeviceOrData(request, callback))
    return;

  RDMResponse *response = GetResponseFromData(
        request,
        reinterpret_cast<const uint8_t*>(value.data()),
        value.size());
  RunRDMCallback(callback, response);
  delete request;
}


/*
 * Handle getting/setting the personality.
 */
void DummyResponder::HandlePersonality(const ola::rdm::RDMRequest *request,
                                       ola::rdm::RDMCallback *callback) {
  RDMResponse *response;
  if (request->SubDevice()) {
    response = NackWithReason(request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    // do set
    if (request->ParamDataSize() != 1) {
      response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
    } else {
      uint8_t personality = *request->ParamData() - 1;
      if (personality >= PERSONALITY_COUNT) {
        response = NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
      } else if (m_start_address + PERSONALITIES[personality].footprint - 1 >
          DMX_UNIVERSE_SIZE) {
        response = NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
      } else {
        m_personality = personality;
        response = new ola::rdm::RDMSetResponse(
          request->DestinationUID(),
          request->SourceUID(),
          request->TransactionNumber(),
          ola::rdm::RDM_ACK,
          0,
          request->SubDevice(),
          request->ParamId(),
          NULL,
          0);
      }
    }
  } else {
    if (request->ParamDataSize()) {
      response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
    } else {
      struct personality_info_s {
        uint8_t personality;
        uint8_t total;
      } __attribute__((packed));

      struct personality_info_s personality_info;
      personality_info.personality = m_personality + 1;
      personality_info.total = PERSONALITY_COUNT;
      response = GetResponseFromData(
        request,
        reinterpret_cast<const uint8_t*>(&personality_info),
        sizeof(personality_info));
    }
  }
  if (request->DestinationUID().IsBroadcast()) {
    vector<string> packets;
    callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
    delete response;
  } else {
    RunRDMCallback(callback, response);
  }
  delete request;
}


/*
 * Handle getting the personality description.
 */
void DummyResponder::HandlePersonalityDescription(
    const ola::rdm::RDMRequest *request,
    ola::rdm::RDMCallback *callback) {
  if (request->DestinationUID().IsBroadcast()) {
    delete request;
    vector<string> packets;
    callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
    return;
  }

  RDMResponse *response = NULL;
  uint8_t personality = 0;
  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    response = NackWithReason(request, ola::rdm::NR_UNSUPPORTED_COMMAND_CLASS);
  } else if (request->SubDevice()) {
    response = NackWithReason(request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->ParamDataSize() != 1) {
    response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  } else {
    personality = *request->ParamData() - 1;
    if (personality >= PERSONALITY_COUNT) {
      response = NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
    }
  }

  if (!response) {
    struct personality_description_s {
      uint8_t personality;
      uint16_t slots_required;
      char description[32];
    } __attribute__((packed));

    struct personality_description_s personality_description;
    personality_description.personality = personality + 1;
    personality_description.slots_required =
      HostToNetwork(PERSONALITIES[personality].footprint);
    strncpy(personality_description.description,
            PERSONALITIES[personality].description,
            sizeof(personality_description.description));

    response = GetResponseFromData(
        request,
        reinterpret_cast<uint8_t*>(&personality_description),
        sizeof(personality_description));
  }
  RunRDMCallback(callback, response);
  delete request;
}


/*
 * Handle getting/setting the dmx start address
 */
void DummyResponder::HandleDmxStartAddress(const RDMRequest *request,
                                           ola::rdm::RDMCallback *callback) {
  RDMResponse *response;
  if (request->SubDevice()) {
    response = NackWithReason(request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    // do set
    if (request->ParamDataSize() != sizeof(m_start_address)) {
      response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
    } else {
      uint16_t address =
        NetworkToHost(*(reinterpret_cast<uint16_t*>(request->ParamData())));
      uint16_t end_address = (
          DMX_UNIVERSE_SIZE - PERSONALITIES[m_personality].footprint + 1);
      if (address == 0 || address > end_address) {
        response = NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
      } else {
        m_start_address = address;
        response = new ola::rdm::RDMSetResponse(
          request->DestinationUID(),
          request->SourceUID(),
          request->TransactionNumber(),
          ola::rdm::RDM_ACK,
          0,
          request->SubDevice(),
          request->ParamId(),
          NULL,
          0);
      }
    }
  } else {
    if (request->ParamDataSize()) {
      response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
    } else {
      uint16_t address = HostToNetwork(m_start_address);
      response = GetResponseFromData(
        request,
        reinterpret_cast<const uint8_t*>(&address),
        sizeof(address));
    }
  }
  if (request->DestinationUID().IsBroadcast()) {
    vector<string> packets;
    delete response;
    callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
  } else {
    RunRDMCallback(callback, response);
  }
  delete request;
}


/*
 * Handle turning identify on/off
 */
void DummyResponder::HandleIdentifyDevice(const RDMRequest *request,
                                          ola::rdm::RDMCallback *callback) {
  RDMResponse *response;
  if (request->SubDevice()) {
    response = NackWithReason(request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    // do set
    if (request->ParamDataSize() != sizeof(m_identify_mode)) {
      response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
    } else {
      uint8_t mode = *request->ParamData();
      if (mode == 0 || mode == 1) {
        m_identify_mode = mode;
        OLA_INFO << "Dummy device, identify mode " << (
            m_identify_mode ? "on" : "off");
        response = new ola::rdm::RDMSetResponse(
          request->DestinationUID(),
          request->SourceUID(),
          request->TransactionNumber(),
          ola::rdm::RDM_ACK,
          0,
          request->SubDevice(),
          request->ParamId(),
          NULL,
          0);
      } else {
        response = NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
      }
    }
  } else {
    if (request->ParamDataSize()) {
      response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
    } else {
      response = GetResponseFromData(
        request,
        &m_identify_mode,
        sizeof(m_identify_mode));
    }
  }
  if (request->DestinationUID().IsBroadcast()) {
    vector<string> packets;
    delete response;
    callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
  } else {
    RunRDMCallback(callback, response);
  }
  delete request;
}

/**
 * Check for the following:
 *   - the callback was non-null
 *   - broadcast request
 *   - request with a sub device set
 *   - request with data
 * And return the correct NACK reason
 * @returns true is this request was ok, false if we nack'ed it
 */
bool DummyResponder::CheckForBroadcastSubdeviceOrData(
    const ola::rdm::RDMRequest *request,
    ola::rdm::RDMCallback *callback) {
  if (!callback) {
    delete request;
    return false;
  }

  if (request->DestinationUID().IsBroadcast()) {
    delete request;
    vector<string> packets;
    callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
    return false;
  }

  RDMResponse *response = NULL;
  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    response = NackWithReason(request, ola::rdm::NR_UNSUPPORTED_COMMAND_CLASS);
  } else if (request->SubDevice()) {
    response = NackWithReason(request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->ParamDataSize()) {
    response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  if (response) {
    RunRDMCallback(callback, response);
    delete request;
    return false;
  }
  return true;
}


/**
 * Run the RDM callback with a response. This takes care of creating the fake
 * raw data.
 */
void DummyResponder::RunRDMCallback(ola::rdm::RDMCallback *callback,
                                    ola::rdm::RDMResponse *response) {
  string raw_response;
  response->Pack(&raw_response);
  vector<string> packets;
  packets.push_back(raw_response);
  callback->Run(ola::rdm::RDM_COMPLETED_OK, response, packets);
}
}  // dummy
}  // plugin
}  // ola
