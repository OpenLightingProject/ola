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
 *
 * DummyPort.cpp
 * The Dummy Port for ola
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <iostream>
#include <string>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/dummy/DummyPort.h"

namespace ola {
namespace plugin {
namespace dummy {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using ola::rdm::GetResponseWithData;
using ola::rdm::NackWithReason;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;

/*
 * Write operation
 * @param  data  pointer to the dmx data
 * @param  length  the length of the data
 */
bool DummyPort::WriteDMX(const DmxBuffer &buffer,
                         uint8_t priority) {
  (void) priority;
  m_buffer = buffer;
  stringstream str;
  string data = buffer.Get();

  str << "Dummy port: got " << buffer.Size() << " bytes: ";
  for (unsigned int i = 0; i < 10 && i < data.size(); i++)
    str << "0x" << std::hex << 0 + (uint8_t) data.at(i) << " ";
  OLA_INFO << str.str();
  return true;
}


/*
 * This returns a single device
 */
void DummyPort::RunRDMDiscovery() {
  ola::rdm::UID uid(OPEN_LIGHTING_ESTA_CODE, 0xffffff00);
  ola::rdm::UIDSet uid_set;
  uid_set.AddUID(uid);
  NewUIDList(uid_set);
}


/*
 * Handle an RDM Request
 */
bool DummyPort::HandleRDMRequest(const RDMRequest *request) {
  switch (request->ParamId()) {
    case ola::rdm::PID_SUPPORTED_PARAMETERS:
      return HandleSupportedParams(request);
    case ola::rdm::PID_DEVICE_INFO:
      return HandleDeviceInfo(request);
    case ola::rdm::PID_PRODUCT_DETAIL_ID_LIST:
      return HandleProductDetailList(request);
    case ola::rdm::PID_MANUFACTURER_LABEL:
      return HandleStringResponse(request, "Open Lighting");
    case ola::rdm::PID_DEVICE_LABEL:
      return HandleStringResponse(request, "Dummy RDM Device");
    case ola::rdm::PID_DEVICE_MODEL_DESCRIPTION:
      return HandleStringResponse(request, "Dummy Model");
    case ola::rdm::PID_SOFTWARE_VERSION_LABEL:
      return HandleStringResponse(request, "Dummy Software Version");
    case ola::rdm::PID_DMX_START_ADDRESS:
      return HandleDmxStartAddress(request);
    default:
      return HandleUnknownPacket(request);
  }
}


bool DummyPort::HandleUnknownPacket(const RDMRequest *request) {
  // no responses for broadcasts
  if (!request->DestinationUID().IsBroadcast()) {
    RDMResponse *response = NackWithReason(request, ola::rdm::NR_UNKNOWN_PID);
    HandleRDMResponse(response);
  }
  delete request;
  return true;
}


bool DummyPort::HandleSupportedParams(const RDMRequest *request) {
  if (request->DestinationUID().IsBroadcast()) {
    delete request;
    return true;
  }

  RDMResponse *response;
  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    response = NackWithReason(request, ola::rdm::NR_UNSUPPORTED_COMMAND_CLASS);
  } else if (request->SubDevice()) {
    response = NackWithReason(request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->ParamDataSize()) {
    response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  } else {
    uint16_t supported_params[] = {
      ola::rdm::PID_SUPPORTED_PARAMETERS,
      ola::rdm::PID_DEVICE_INFO,
      ola::rdm::PID_PRODUCT_DETAIL_ID_LIST,
      ola::rdm::PID_DEVICE_MODEL_DESCRIPTION,
      ola::rdm::PID_MANUFACTURER_LABEL,
      ola::rdm::PID_DEVICE_LABEL,
      ola::rdm::PID_SOFTWARE_VERSION_LABEL,
      ola::rdm::PID_DMX_START_ADDRESS
    };

    for (unsigned int i = 0; i < sizeof(supported_params) / 2; i++)
      supported_params[i] = HostToNetwork(supported_params[i]);

    response = GetResponseWithData(
      request,
      reinterpret_cast<uint8_t*>(supported_params),
      sizeof(supported_params));
  }
  HandleRDMResponse(response);
  delete request;
  return true;
}


bool DummyPort::HandleDeviceInfo(const RDMRequest *request) {
  if (request->DestinationUID().IsBroadcast()) {
    delete request;
    return true;
  }

  RDMResponse *response;
  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    response = NackWithReason(request, ola::rdm::NR_UNSUPPORTED_COMMAND_CLASS);
  } else if (request->SubDevice()) {
    response = NackWithReason(request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->ParamDataSize()) {
    response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  } else {
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
      HostToNetwork(static_cast<uint16_t>(DUMMY_DMX_FOOTPRINT));
    device_info.current_personality = 1;
    device_info.personality_count = 1;
    device_info.dmx_start_address = HostToNetwork(m_start_address);
    device_info.sub_device_count = 0;
    device_info.sensor_count = 0;
    response = GetResponseWithData(request,
                                   reinterpret_cast<uint8_t*>(&device_info),
                                   sizeof(device_info));
  }
  HandleRDMResponse(response);
  delete request;
  return true;
}


/**
 * Handle a request for PID_PRODUCT_DETAIL_ID_LIST
 */
bool DummyPort::HandleProductDetailList(const RDMRequest *request) {
  if (request->DestinationUID().IsBroadcast()) {
    delete request;
    return true;
  }

  RDMResponse *response;
  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    response = NackWithReason(request, ola::rdm::NR_UNSUPPORTED_COMMAND_CLASS);
  } else if (request->SubDevice()) {
    response = NackWithReason(request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->ParamDataSize()) {
    response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  } else {
    uint16_t product_details[] = {
      ola::rdm::PRODUCT_DETAIL_TEST,
      ola::rdm::PRODUCT_DETAIL_OTHER
    };

    for (unsigned int i = 0; i < sizeof(product_details) / 2; i++)
      product_details[i] = HostToNetwork(product_details[i]);

    response = GetResponseWithData(
        request,
        reinterpret_cast<uint8_t*>(&product_details),
        sizeof(product_details));
  }
  HandleRDMResponse(response);
  delete request;
  return true;
}


/*
 * Handle a request that returns a string
 */
bool DummyPort::HandleStringResponse(const ola::rdm::RDMRequest *request,
                                     const string &value) {
  if (request->DestinationUID().IsBroadcast()) {
    delete request;
    return true;
  }

  RDMResponse *response;
  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    response = NackWithReason(request, ola::rdm::NR_UNSUPPORTED_COMMAND_CLASS);
  } else if (request->SubDevice()) {
    response = NackWithReason(request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->ParamDataSize()) {
    response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  } else {
    response = GetResponseWithData(
        request,
        reinterpret_cast<const uint8_t*>(value.data()),
        value.size());
  }
  HandleRDMResponse(response);
  delete request;
  return true;
}


/*
 * Handle getting/setting the dmx start address
 */
bool DummyPort::HandleDmxStartAddress(const RDMRequest *request) {
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
      if (address == 0 || address > DMX_UNIVERSE_SIZE - DUMMY_DMX_FOOTPRINT) {
        response = NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
      } else {
        m_start_address = address;
        response = new ola::rdm::RDMSetResponse(
          request->DestinationUID(),
          request->SourceUID(),
          request->TransactionNumber(),
          ola::rdm::ACK,
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
      response = GetResponseWithData(
        request,
        reinterpret_cast<const uint8_t*>(&address),
        sizeof(address));
    }
  }
  if (request->DestinationUID().IsBroadcast()) {
    delete response;
  } else {
    HandleRDMResponse(response);
  }
    delete request;
  return true;
}
}  // dummy
}  // plugin
}  // ola
