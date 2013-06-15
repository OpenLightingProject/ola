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
 * DummyRDMDevice.cpp
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <iostream>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/RDMManufacturerPIDs.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/dummy/DummyRDMDevice.h"

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


const DummyRDMDevice::personality_info DummyRDMDevice::PERSONALITIES[] = {
  {0, "Personality 1"},
  {5, "Personality 2"},
  {10, "Personality 3"},
  {20, "Personality 4"},
};

const unsigned int DummyRDMDevice::PERSONALITY_COUNT = (
  sizeof(DummyRDMDevice::PERSONALITIES) /
  sizeof(DummyRDMDevice::personality_info));


/*
 * Handle an RDM Request
 */
void DummyRDMDevice::SendRDMRequest(const ola::rdm::RDMRequest *request,
                                    ola::rdm::RDMCallback *callback) {
  vector<string> packets;
  if (request->CommandClass() == ola::rdm::RDMCommand::DISCOVER_COMMAND) {
    callback->Run(ola::rdm::RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED,
                  NULL,
                  packets);
    delete request;
    return;
  }

  if (!request->DestinationUID().DirectedToUID(m_uid)) {
    if (!request->DestinationUID().IsBroadcast())
      OLA_WARN << "Dummy responder received request for the wrong UID, " <<
        "expected " << m_uid << ", got " << request->DestinationUID();

    callback->Run(
        (request->DestinationUID().IsBroadcast() ?
          ola::rdm::RDM_WAS_BROADCAST :
          ola::rdm::RDM_TIMEOUT),
        NULL,
        packets);
    delete request;
    return;
  }

  switch (request->ParamId()) {
    case ola::rdm::PID_SUPPORTED_PARAMETERS:
      HandleSupportedParams(request, callback);
      break;
    case ola::rdm::PID_DEVICE_INFO:
      HandleDeviceInfo(request, callback);
      break;
    case ola::rdm::PID_PARAMETER_DESCRIPTION:
      HandleParamDescription(request, callback);
      break;
    case ola::rdm::PID_FACTORY_DEFAULTS:
      HandleFactoryDefaults(request, callback);
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
    case ola::rdm::PID_LAMP_STRIKES:
      HandleLampStrikes(request, callback);
      break;
    case ola::rdm::PID_IDENTIFY_DEVICE:
      HandleIdentifyDevice(request, callback);
      break;
    case ola::rdm::PID_REAL_TIME_CLOCK:
      HandleRealTimeClock(request, callback);
      break;
    case ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION:
      HandleStringResponse(request, callback, VERSION);
      break;
    default:
      HandleUnknownPacket(request, callback);
  }
}


void DummyRDMDevice::HandleUnknownPacket(const RDMRequest *request,
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


void DummyRDMDevice::HandleSupportedParams(const RDMRequest *request,
                                           ola::rdm::RDMCallback *callback) {
  if (!CheckForBroadcastSubdeviceOrData(request, callback))
    return;

  uint16_t supported_params[] = {
    ola::rdm::PID_DEVICE_LABEL,
    ola::rdm::PID_FACTORY_DEFAULTS,
    ola::rdm::PID_DEVICE_MODEL_DESCRIPTION,
    ola::rdm::PID_DMX_PERSONALITY,
    ola::rdm::PID_DMX_PERSONALITY_DESCRIPTION,
    ola::rdm::PID_MANUFACTURER_LABEL,
    ola::rdm::PID_PRODUCT_DETAIL_ID_LIST,
    ola::rdm::PID_LAMP_STRIKES,
    ola::rdm::PID_REAL_TIME_CLOCK,
    ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION
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


void DummyRDMDevice::HandleParamDescription(const RDMRequest *request,
                                            ola::rdm::RDMCallback *callback) {
  RDMResponse *response = NULL;
  if (!CheckForBroadcastSubdeviceOrData(request, callback, sizeof(uint16_t)))
    return;

  // Check that it's MANUFACTURER_PID_CODE_VERSION being requested
  uint16_t parameter_id;
  memcpy(reinterpret_cast<uint8_t*>(&parameter_id),
         request->ParamData(),
         sizeof(parameter_id));
  parameter_id = NetworkToHost(parameter_id);
  if (parameter_id != ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION) {
    OLA_WARN << "Dummy responder received param description request with "
      "unknown PID, expected " << ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION
      << ", got " << parameter_id;
      response = NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
  } else {
    struct parameter_description_s {
      uint16_t pid;
      uint8_t pdl_size;
      uint8_t data_type;
      uint8_t command_class;
      uint8_t type;
      uint8_t unit;
      uint8_t prefix;
      uint32_t min_value;
      uint32_t default_value;
      uint32_t max_value;
      char description[ola::rdm::MAX_RDM_STRING_LENGTH];
    } __attribute__((packed));

    struct parameter_description_s param_description;
    param_description.pid = HostToNetwork(
        static_cast<uint16_t>(ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION));
    param_description.pdl_size = HostToNetwork(
        static_cast<uint8_t>(ola::rdm::MAX_RDM_STRING_LENGTH));
    param_description.data_type = HostToNetwork(
        static_cast<uint8_t>(ola::rdm::DS_ASCII));
    param_description.command_class = HostToNetwork(
        static_cast<uint8_t>(ola::rdm::CC_GET));
    param_description.type = 0;
    param_description.unit = HostToNetwork(
        static_cast<uint8_t>(ola::rdm::UNITS_NONE));
    param_description.prefix = HostToNetwork(
        static_cast<uint8_t>(ola::rdm::PREFIX_NONE));
    param_description.min_value = 0;
    param_description.default_value = 0;
    param_description.max_value = 0;
    strncpy(param_description.description,
            "Code Version",
            ola::rdm::MAX_RDM_STRING_LENGTH);
    response = GetResponseFromData(
        request,
        reinterpret_cast<uint8_t*>(&param_description),
        sizeof(param_description));
  }
  RunRDMCallback(callback, response);
  delete request;
}


void DummyRDMDevice::HandleDeviceInfo(const RDMRequest *request,
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
  device_info.dmx_footprint = HostToNetwork(Footprint());
  device_info.current_personality = m_personality + 1;
  device_info.personality_count = PERSONALITY_COUNT;
  device_info.dmx_start_address = device_info.dmx_footprint ?
    HostToNetwork(m_start_address) : 0xffff;
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
 * Reset to factory defaults
 */
void DummyRDMDevice::HandleFactoryDefaults(const ola::rdm::RDMRequest *request,
                                           ola::rdm::RDMCallback *callback) {
  if (!CheckForSubdevice(request, callback))
    return;

  RDMResponse *response = NULL;
  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    // do set
    if (request->ParamDataSize()) {
      response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
    } else {
      m_start_address = 1;
      m_personality = 1;
      m_identify_mode = 0;

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
  } else {
    if (request->ParamDataSize()) {
      response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
    } else {
      uint8_t using_defaults = (
          m_start_address == 1 &&
          m_personality == 1 &&
          m_identify_mode == false);
      response = GetResponseFromData(
        request,
        &using_defaults,
        sizeof(using_defaults));
    }
  }

  if (!CheckForBroadcast(request, callback))
    return;  // Broadcast request, no response

  RunRDMCallback(callback, response);
  delete request;
}


/**
 * Handle a request for PID_PRODUCT_DETAIL_ID_LIST
 */
void DummyRDMDevice::HandleProductDetailList(const RDMRequest *request,
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
void DummyRDMDevice::HandleStringResponse(const ola::rdm::RDMRequest *request,
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
void DummyRDMDevice::HandlePersonality(const ola::rdm::RDMRequest *request,
                                       ola::rdm::RDMCallback *callback) {
  if (!CheckForSubdevice(request, callback))
    return;

  RDMResponse *response = NULL;
  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    // do set
    if (request->ParamDataSize() != 1) {
      response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
    } else {
      uint8_t personality = *request->ParamData();
      if (personality > PERSONALITY_COUNT || personality == 0) {
        response = NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
      } else if (m_start_address + PERSONALITIES[personality - 1].footprint - 1
                 > DMX_UNIVERSE_SIZE) {
        response = NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
      } else {
        m_personality = personality - 1;
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

  if (!CheckForBroadcast(request, callback))
    return;  // Broadcast request, no response

  RunRDMCallback(callback, response);
  delete request;
}


/*
 * Handle getting the personality description.
 */
void DummyRDMDevice::HandlePersonalityDescription(
    const ola::rdm::RDMRequest *request,
    ola::rdm::RDMCallback *callback) {
  if (!CheckForBroadcastSubdeviceOrData(request, callback, 1))
    return;

  RDMResponse *response = NULL;
  uint8_t personality = 0;
  personality = *request->ParamData() - 1;
  if (personality >= PERSONALITY_COUNT) {
    response = NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
  } else {
    struct personality_description_s {
      uint8_t personality;
      uint16_t slots_required;
      char description[ola::rdm::MAX_RDM_STRING_LENGTH];
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
void DummyRDMDevice::HandleDmxStartAddress(const RDMRequest *request,
                                           ola::rdm::RDMCallback *callback) {
  if (!CheckForSubdevice(request, callback))
    return;

  RDMResponse *response = NULL;
  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    // do set
    if (request->ParamDataSize() != sizeof(m_start_address)) {
      response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
    } else {
      uint16_t address =
        NetworkToHost(*(reinterpret_cast<uint16_t*>(request->ParamData())));
      uint16_t end_address = DMX_UNIVERSE_SIZE - Footprint() + 1;
      if (address == 0 || address > end_address) {
        response = NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
      } else if (Footprint() == 0) {
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
      if (Footprint() == 0)
        address = 0xffff;
      response = GetResponseFromData(
        request,
        reinterpret_cast<const uint8_t*>(&address),
        sizeof(address));
    }
  }

  if (!CheckForBroadcast(request, callback))
    return;  // Broadcast request, no response

  RunRDMCallback(callback, response);
  delete request;
}


/**
 * Handle a LAMP_STRIKES request
 */
void DummyRDMDevice::HandleLampStrikes(const ola::rdm::RDMRequest *request,
                                       ola::rdm::RDMCallback *callback) {
  if (!CheckForSubdevice(request, callback))
    return;

  RDMResponse *response = NULL;
  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    // do set
    if (request->ParamDataSize() != sizeof(m_lamp_strikes)) {
      response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
    } else {
      m_lamp_strikes =
        NetworkToHost(*(reinterpret_cast<uint32_t*>(request->ParamData())));
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
  } else {
    if (request->ParamDataSize()) {
      response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
    } else {
      uint32_t strikes = HostToNetwork(m_lamp_strikes);
      response = GetResponseFromData(
        request,
        reinterpret_cast<const uint8_t*>(&strikes),
        sizeof(strikes));
    }
  }

  if (!CheckForBroadcast(request, callback))
    return;  // Broadcast request, no response

  RunRDMCallback(callback, response);
  delete request;
}



/*
 * Handle turning identify on/off
 */
void DummyRDMDevice::HandleIdentifyDevice(const RDMRequest *request,
                                          ola::rdm::RDMCallback *callback) {
  if (!CheckForSubdevice(request, callback))
    return;

  RDMResponse *response = NULL;
  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
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

  if (!CheckForBroadcast(request, callback))
    return;  // Broadcast request, no response

  RunRDMCallback(callback, response);
  delete request;
}


/**
 * Handle the Real Time Clock
 */
void DummyRDMDevice::HandleRealTimeClock(const RDMRequest *request,
                                         ola::rdm::RDMCallback *callback) {
  if (!CheckForBroadcastSubdeviceOrData(request, callback))
    return;

  struct clock_s {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
  } __attribute__((packed));

  time_t now;
  now = time(NULL);
  struct tm tm_now;
  localtime_r(&now, &tm_now);

  struct clock_s clock;
  clock.year = HostToNetwork(static_cast<uint16_t>(1900 + tm_now.tm_year));
  clock.month = tm_now.tm_mon + 1;
  clock.day = tm_now.tm_mday;
  clock.hour = tm_now.tm_hour;
  clock.minute = tm_now.tm_min;
  clock.second = tm_now.tm_sec;

  RDMResponse *response = GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&clock),
      sizeof(clock));
  RunRDMCallback(callback, response);
  delete request;
}


/**
 * Check for the following:
 *   - the callback was non-null
 *   - broadcast request
 *   - set requests
 *   - request with a sub device set
 *   - request with data that isn't the expected size (defaults to 0)
 * And return the correct NACK reason
 * @returns true is this request was ok, false if we nack'ed it
 */
bool DummyRDMDevice::CheckForBroadcastSubdeviceOrData(
    const ola::rdm::RDMRequest *request,
    ola::rdm::RDMCallback *callback,
    uint8_t parameter_size) {
  if (!CheckForBroadcastOrSubdevice(request, callback))
    return false;

  RDMResponse *response = NULL;
  if (request->CommandClass() != ola::rdm::RDMCommand::GET_COMMAND) {
    response = NackWithReason(request, ola::rdm::NR_UNSUPPORTED_COMMAND_CLASS);
  } else if (request->ParamDataSize() != parameter_size) {
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
 * Check for the following:
 *   - the callback was non-null
 *   - broadcast request
 *   - request with a sub device set
 * And return the correct NACK reason
 * @returns true is this request was ok, false if we nack'ed it
 */
bool DummyRDMDevice::CheckForBroadcastOrSubdevice(
    const ola::rdm::RDMRequest *request,
    ola::rdm::RDMCallback *callback) {
  if (!CheckForBroadcast(request, callback)) {
    return false;
  } else if (!CheckForSubdevice(request, callback)) {
    return false;
  }
  return true;
}


/**
 * Check for the following:
 *   - the callback was non-null
 *   - broadcast request
 * And return the correct NACK reason
 * @returns true is this request was ok, false if we nack'ed it
 */
bool DummyRDMDevice::CheckForBroadcast(
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
  return true;
}


/**
 * Check for the following:
 *   - the callback was non-null
 *   - request with a sub device set
 * And return the correct NACK reason
 * @returns true is this request was ok, false if we nack'ed it
 */
bool DummyRDMDevice::CheckForSubdevice(
    const ola::rdm::RDMRequest *request,
    ola::rdm::RDMCallback *callback) {
  if (!callback) {
    delete request;
    return false;
  }

  if (request->SubDevice()) {
    RDMResponse *response = NackWithReason(
        request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
    RunRDMCallback(callback, response);
    delete request;
    return false;
  }
  return true;
}


/**
 * Run the RDM callback with a response.
 */
void DummyRDMDevice::RunRDMCallback(ola::rdm::RDMCallback *callback,
                                    ola::rdm::RDMResponse *response) {
  vector<string> packets;
  callback->Run(ola::rdm::RDM_COMPLETED_OK, response, packets);
}
}  // namespace dummy
}  // namespace plugin
}  // namespace ola
