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
 * SPIPort.h
 * The SPI plugin for ola
 * Copyright (C) 2013 Simon Newton
 */

#include <errno.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <sys/ioctl.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SocketCloser.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/stl/STLUtils.h"

#include "plugins/spi/SPIPort.h"

namespace ola {
namespace plugin {
namespace spi {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using ola::rdm::RDMCallback;
using ola::rdm::RDMCommand;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::auto_ptr;

const uint16_t SPIOutputPort::SPI_DELAY = 0;
const uint32_t SPIOutputPort::SPI_SPEED = 1000000;
const uint8_t SPIOutputPort::SPI_BITS_PER_WORD = 8;
const uint8_t SPIOutputPort::SPI_MODE = 0;
const uint8_t SPIOutputPort::PERSONALITY_WS2801_INDIVIDUAL = 0;
const uint8_t SPIOutputPort::PERSONALITY_WS2801_SIMULATANEOUS = 1;
const uint8_t SPIOutputPort::PERSONALITY_LAST = 2;
const uint16_t SPIOutputPort::CHANNELS_PER_PIXEL = 3;



SPIOutputPort::SPIOutputPort(SPIDevice *parent, const string &spi_device,
                             const UID &uid, uint8_t pixel_count)
    : BasicOutputPort(parent, 0, true),
      m_device_path(spi_device),
      m_spi_device_name(spi_device),
      m_uid(uid),
      m_pixel_count(pixel_count),
      m_fd(-1),
      m_personality(PERSONALITY_WS2801_INDIVIDUAL),
      m_start_address(1),
      m_identify_mode(false) {
  size_t pos = spi_device.find_last_of("/");
  if (pos != string::npos)
    m_spi_device_name = spi_device.substr(pos + 1);

  m_personality_manager.AddPersonality(m_pixel_count * CHANNELS_PER_PIXEL,
                                        "WS2801 Individual Control");
  m_personality_manager.AddPersonality(CHANNELS_PER_PIXEL,
                                       "WS2801 Combined Control");
}


SPIOutputPort::~SPIOutputPort() {
  if (m_fd >= 0)
    close(m_fd);
}


/**
 * Open the SPI device
 */
bool SPIOutputPort::Init() {
  int fd = open(m_device_path.c_str(), O_RDWR);
  ola::network::SocketCloser closer(fd);
  if (fd < 0) {
    OLA_WARN << "Failed to open " << m_device_path << " : " << strerror(errno);
    return false;
  }

  uint8_t spi_mode = SPI_MODE;
  if (ioctl(fd, SPI_IOC_WR_MODE, &spi_mode) < 0) {
    OLA_WARN << "Failed to set SPI_IOC_WR_MODE for " << m_device_path;
    return false;
  }

  uint8_t spi_bits_per_word = SPI_BITS_PER_WORD;
  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits_per_word) < 0) {
    OLA_WARN << "Failed to set SPI_IOC_WR_BITS_PER_WORD for " << m_device_path;
    return false;
  }

  uint32_t spi_speed = SPI_SPEED;
  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) < 0) {
    OLA_WARN << "Failed to set SPI_IOC_WR_MAX_SPEED_HZ for " << m_device_path;
    return false;
  }
  m_fd = closer.Release();
  return true;
}


/*
 * Send DMX data over SPI.
 */
bool SPIOutputPort::WriteDMX(const DmxBuffer &buffer, uint8_t) {
  if (m_fd < 0)
    return false;

  unsigned int length = m_pixel_count * CHANNELS_PER_PIXEL;
  uint8_t *output_data = new uint8_t[length];

  if (m_personality == PERSONALITY_WS2801_INDIVIDUAL) {
    buffer.GetRange(m_start_address - 1, output_data, &length);
  } else {
    unsigned int pixel_data_length = CHANNELS_PER_PIXEL;
    uint8_t pixel_data[CHANNELS_PER_PIXEL];
    buffer.GetRange(m_start_address - 1, pixel_data,
        &pixel_data_length);
    if (pixel_data_length != CHANNELS_PER_PIXEL) {
      OLA_INFO << "Insufficient DMX data, required " << CHANNELS_PER_PIXEL
               << ", got " << pixel_data_length;
      // insufficient data
      delete[] output_data;
      return true;
    }
    for (unsigned int i = 0; i < m_pixel_count; i++) {
      memcpy(output_data + (i * CHANNELS_PER_PIXEL), pixel_data,
             pixel_data_length);
    }
  }

  struct spi_ioc_transfer spi;
  memset(&spi, 0, sizeof(spi));
  spi.tx_buf = reinterpret_cast<__u64>(output_data);
  spi.len = length;
  int bytes_written = ioctl(m_fd, SPI_IOC_MESSAGE(1), &spi);
  delete[] output_data;
  if (bytes_written != static_cast<int>(length)) {
    OLA_WARN << "Failed to write all the SPI data: " << strerror(errno);
    return false;
  }
  return true;
}


void SPIOutputPort::RunFullDiscovery(RDMDiscoveryCallback *callback) {
  UIDSet uids;
  uids.AddUID(m_uid);
  callback->Run(uids);
}


void SPIOutputPort::RunIncrementalDiscovery(RDMDiscoveryCallback *callback) {
  UIDSet uids;
  uids.AddUID(m_uid);
  callback->Run(uids);
}


void SPIOutputPort::SendRDMRequest(const RDMRequest *request_ptr,
                                   RDMCallback *callback) {
  auto_ptr<const RDMRequest> request(request_ptr);
  if (request->CommandClass() == RDMCommand::DISCOVER_COMMAND) {
    RunRDMCallback(callback, ola::rdm::RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED);
    return;
  }

  if (!request->DestinationUID().DirectedToUID(m_uid)) {
    if (!request->DestinationUID().IsBroadcast())
      OLA_WARN << "SPI responder received request for the wrong UID, " <<
        "expected " << m_uid << ", got " << request->DestinationUID();
    RunRDMCallback(callback,
                   (request->DestinationUID().IsBroadcast() ?
                     ola::rdm::RDM_WAS_BROADCAST :
                     ola::rdm::RDM_TIMEOUT));
    return;
  }

  request.release();
  switch (request_ptr->ParamId()) {
    case ola::rdm::PID_SUPPORTED_PARAMETERS:
      HandleSupportedParams(request_ptr, callback);
      break;
    case ola::rdm::PID_DEVICE_INFO:
      HandleDeviceInfo(request_ptr, callback);
      break;
    case ola::rdm::PID_PRODUCT_DETAIL_ID_LIST:
      HandleProductDetailList(request_ptr, callback);
      break;
    case ola::rdm::PID_MANUFACTURER_LABEL:
      HandleStringResponse(request_ptr, callback, "Open Lighting");
      break;
    case ola::rdm::PID_DEVICE_LABEL:
      HandleStringResponse(request_ptr, callback, "Default SPI Device");
      break;
    case ola::rdm::PID_DEVICE_MODEL_DESCRIPTION:
      HandleStringResponse(request_ptr, callback, "OLA SPI Device");
      break;
    case ola::rdm::PID_SOFTWARE_VERSION_LABEL:
      HandleStringResponse(request_ptr, callback, "OLA SPI Plugin v1");
      break;
    case ola::rdm::PID_DMX_PERSONALITY:
      HandlePersonality(request_ptr, callback);
      break;
    case ola::rdm::PID_DMX_PERSONALITY_DESCRIPTION:
      HandlePersonalityDescription(request_ptr, callback);
      break;
    case ola::rdm::PID_DMX_START_ADDRESS:
      HandleDmxStartAddress(request_ptr, callback);
      break;
    case ola::rdm::PID_IDENTIFY_DEVICE:
      HandleIdentifyDevice(request_ptr, callback);
      break;
    default:
      HandleUnknownPacket(request_ptr, callback);
  }
}


/**
 * The current personalities' footprint
 */
uint16_t SPIOutputPort::Footprint() const {
  return PersonalityFootprint(m_personality);
}


/**
 * The footprint for a particular personality.
 */
uint16_t SPIOutputPort::PersonalityFootprint(uint8_t personality) const {
  if (personality == PERSONALITY_WS2801_INDIVIDUAL) {
    return m_pixel_count * CHANNELS_PER_PIXEL;
  } else {
    return CHANNELS_PER_PIXEL;
  }
}


string SPIOutputPort::PersonalityDescription(uint8_t personality) const {
  if (personality == PERSONALITY_WS2801_INDIVIDUAL) {
    return "WS2801 Individual Control";
  } else {
    return "WS2801 Combined Control";
  }
}

void SPIOutputPort::HandleUnknownPacket(const RDMRequest *request_ptr,
                                        RDMCallback *callback) {
  auto_ptr<const RDMRequest> request(request_ptr);
  if (request->DestinationUID().IsBroadcast()) {
    // no responses for broadcasts
    RunRDMCallback(callback, ola::rdm::RDM_WAS_BROADCAST);
  } else {
    RDMResponse *response = NackWithReason(request.get(),
                                           ola::rdm::NR_UNKNOWN_PID);
    RunRDMCallback(callback, response);
  }
}


void SPIOutputPort::HandleSupportedParams(const RDMRequest *request_ptr,
                                          RDMCallback *callback) {
  auto_ptr<const RDMRequest> request(request_ptr);
  if (!CheckForBroadcastSubdeviceOrData(request.get(), callback))
    return;

  uint16_t supported_params[] = {
    ola::rdm::PID_DEVICE_LABEL,
    ola::rdm::PID_DEVICE_MODEL_DESCRIPTION,
    ola::rdm::PID_DMX_PERSONALITY,
    ola::rdm::PID_DMX_PERSONALITY_DESCRIPTION,
    ola::rdm::PID_MANUFACTURER_LABEL,
    ola::rdm::PID_PRODUCT_DETAIL_ID_LIST,
  };

  for (unsigned int i = 0; i < sizeof(supported_params) / 2; i++)
    supported_params[i] = HostToNetwork(supported_params[i]);

  RDMResponse *response = GetResponseFromData(
      request.get(),
      reinterpret_cast<uint8_t*>(supported_params),
      sizeof(supported_params));

  RunRDMCallback(callback, response);
}


void SPIOutputPort::HandleDeviceInfo(const RDMRequest *request_ptr,
                                      RDMCallback *callback) {
  auto_ptr<const RDMRequest> request(request_ptr);
  if (!CheckForBroadcastSubdeviceOrData(request.get(), callback))
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
  device_info.model = HostToNetwork(static_cast<uint16_t>(2));
  device_info.product_category = HostToNetwork(
      static_cast<uint16_t>(ola::rdm::PRODUCT_CATEGORY_FIXTURE));
  device_info.software_version = HostToNetwork(static_cast<uint32_t>(1));
  device_info.dmx_footprint = HostToNetwork(Footprint());
  device_info.current_personality = m_personality + 1;
  device_info.personality_count = m_personality_manager.PersonalityCount();
  device_info.dmx_start_address = device_info.dmx_footprint ?
    HostToNetwork(m_start_address) : 0xffff;
  device_info.sub_device_count = 0;
  device_info.sensor_count = 0;
  RDMResponse *response = GetResponseFromData(
      request.get(),
      reinterpret_cast<uint8_t*>(&device_info),
      sizeof(device_info));
  RunRDMCallback(callback, response);
}


/**
 * Handle a request for PID_PRODUCT_DETAIL_ID_LIST
 */
void SPIOutputPort::HandleProductDetailList(const RDMRequest *request_ptr,
                                             RDMCallback *callback) {
  auto_ptr<const RDMRequest> request(request_ptr);
  if (!CheckForBroadcastSubdeviceOrData(request.get(), callback))
    return;

  uint16_t product_details[] = {
    ola::rdm::PRODUCT_DETAIL_LED,
  };

  for (unsigned int i = 0; i < sizeof(product_details) / 2; i++)
    product_details[i] = HostToNetwork(product_details[i]);

  RDMResponse *response = GetResponseFromData(
      request.get(),
      reinterpret_cast<uint8_t*>(&product_details),
      sizeof(product_details));
  RunRDMCallback(callback, response);
}


/*
 * Handle a request that returns a string
 */
void SPIOutputPort::HandleStringResponse(const RDMRequest *request_ptr,
                                         RDMCallback *callback,
                                         const string &value) {
  auto_ptr<const RDMRequest> request(request_ptr);
  if (!CheckForBroadcastSubdeviceOrData(request.get(), callback))
    return;

  RDMResponse *response = GetResponseFromData(
        request.get(),
        reinterpret_cast<const uint8_t*>(value.data()),
        value.size());
  RunRDMCallback(callback, response);
}


/*
 * Handle getting/setting the personality.
 */
void SPIOutputPort::HandlePersonality(const RDMRequest *request_ptr,
                                      RDMCallback *callback) {
  auto_ptr<const RDMRequest> request(request_ptr);
  RDMResponse *response;
  if (request->SubDevice()) {
    response = NackWithReason(request.get(),
                              ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    // do set
    if (request->ParamDataSize() != 1) {
      response = NackWithReason(request.get(), ola::rdm::NR_FORMAT_ERROR);
    } else {
      uint8_t personality = *request->ParamData();
      if (personality > PERSONALITY_LAST || personality == 0) {
        response = NackWithReason(request.get(),
                                  ola::rdm::NR_DATA_OUT_OF_RANGE);
      } else if (m_start_address + PersonalityFootprint(personality - 1) - 1
                 > DMX_UNIVERSE_SIZE) {
        response = NackWithReason(request.get(),
                                  ola::rdm::NR_DATA_OUT_OF_RANGE);
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
      response = NackWithReason(request.get(), ola::rdm::NR_FORMAT_ERROR);
    } else {
      struct personality_info_s {
        uint8_t personality;
        uint8_t total;
      } __attribute__((packed));

      struct personality_info_s personality_info = {
        m_personality_manager.ActivePersonalityNumber(),
        m_personality_manager.PersonalityCount()
      };
      response = GetResponseFromData(
        request.get(),
        reinterpret_cast<const uint8_t*>(&personality_info),
        sizeof(personality_info));
    }
  }
  if (request->DestinationUID().IsBroadcast()) {
    RunRDMCallback(callback, ola::rdm::RDM_WAS_BROADCAST);
    delete response;
  } else {
    RunRDMCallback(callback, response);
  }
}


/*
 * Handle getting the personality description.
 */
void SPIOutputPort::HandlePersonalityDescription(const RDMRequest *request_ptr,
                                                 RDMCallback *callback) {
  auto_ptr<const RDMRequest> request(request_ptr);
  if (request->DestinationUID().IsBroadcast()) {
    RunRDMCallback(callback, ola::rdm::RDM_WAS_BROADCAST);
    return;
  }

  RDMResponse *response = NULL;
  uint8_t personality_number = 0;
  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    response = NackWithReason(request.get(),
                              ola::rdm::NR_UNSUPPORTED_COMMAND_CLASS);
  } else if (request->SubDevice()) {
    response = NackWithReason(request.get(),
                              ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->ParamDataSize() != sizeof(personality_number)) {
    response = NackWithReason(request.get(), ola::rdm::NR_FORMAT_ERROR);
  } else {
    personality_number = *request->ParamData();
    const Personality *personality = m_personality_manager.Lookup(
        personality_number);
    if (!personality) {
      response = NackWithReason(request.get(), ola::rdm::NR_DATA_OUT_OF_RANGE);
    } else {
      struct personality_description_s {
        uint8_t personality;
        uint16_t slots_required;
        char description[32];
      } __attribute__((packed));

      struct personality_description_s personality_description = {
        personality_number, HostToNetwork(personality->footprint()), ""
      };
      strncpy(personality_description.description,
              personality->description().c_str(),
              sizeof(personality_description.description));

      response = GetResponseFromData(
          request.get(),
          reinterpret_cast<uint8_t*>(&personality_description),
          sizeof(personality_description));
    }
  }
  RunRDMCallback(callback, response);
}


/*
 * Handle getting/setting the dmx start address
 */
void SPIOutputPort::HandleDmxStartAddress(const RDMRequest *request_ptr,
                                          RDMCallback *callback) {
  auto_ptr<const RDMRequest> request(request_ptr);

  RDMResponse *response;
  if (request->SubDevice()) {
    response = NackWithReason(request.get(),
                              ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    // do set
    if (request->ParamDataSize() != sizeof(m_start_address)) {
      response = NackWithReason(request.get(), ola::rdm::NR_FORMAT_ERROR);
    } else {
      uint16_t address =
        NetworkToHost(*(reinterpret_cast<uint16_t*>(request->ParamData())));
      uint16_t end_address = DMX_UNIVERSE_SIZE - Footprint() + 1;
      if (address == 0 || address > end_address) {
        response = NackWithReason(request.get(),
                                  ola::rdm::NR_DATA_OUT_OF_RANGE);
      } else if (Footprint() == 0) {
        response = NackWithReason(request.get(),
                                  ola::rdm::NR_DATA_OUT_OF_RANGE);
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
      response = NackWithReason(request.get(), ola::rdm::NR_FORMAT_ERROR);
    } else {
      uint16_t address = HostToNetwork(m_start_address);
      if (Footprint() == 0)
        address = 0xffff;
      response = GetResponseFromData(
        request.get(),
        reinterpret_cast<const uint8_t*>(&address),
        sizeof(address));
    }
  }
  if (request->DestinationUID().IsBroadcast()) {
    RunRDMCallback(callback, ola::rdm::RDM_WAS_BROADCAST);
    delete response;
  } else {
    RunRDMCallback(callback, response);
  }
}


/*
 * Handle turning identify on/off
 */
void SPIOutputPort::HandleIdentifyDevice(const RDMRequest *request_ptr,
                                         RDMCallback *callback) {
  auto_ptr<const RDMRequest> request(request_ptr);
  RDMResponse *response;
  if (request->SubDevice()) {
    response = NackWithReason(request.get(),
                              ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    // do set
    if (request->ParamDataSize() != sizeof(m_identify_mode)) {
      response = NackWithReason(request.get(), ola::rdm::NR_FORMAT_ERROR);
    } else {
      uint8_t mode = *request->ParamData();
      if (mode == 0 || mode == 1) {
        m_identify_mode = mode;
        OLA_INFO << "SPI " << m_spi_device_name << " identify mode " << (
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
        response = NackWithReason(request.get(),
                                  ola::rdm::NR_DATA_OUT_OF_RANGE);
      }
    }
  } else {
    if (request->ParamDataSize()) {
      response = NackWithReason(request.get(), ola::rdm::NR_FORMAT_ERROR);
    } else {
      uint8_t identify_mode = m_identify_mode;
      response = GetResponseFromData(
          request.get(),
          reinterpret_cast<const uint8_t*>(&identify_mode),
          sizeof(identify_mode));
    }
  }
  if (request->DestinationUID().IsBroadcast()) {
    RunRDMCallback(callback, ola::rdm::RDM_WAS_BROADCAST);
    delete response;
  } else {
    RunRDMCallback(callback, response);
  }
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
bool SPIOutputPort::CheckForBroadcastSubdeviceOrData(
    const RDMRequest *request,
    RDMCallback *callback) {
  if (!callback) {
    return false;
  }

  if (request->DestinationUID().IsBroadcast()) {
    RunRDMCallback(callback, ola::rdm::RDM_WAS_BROADCAST);
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
    return false;
  }
  return true;
}


/**
 * Run the RDM callback with a response.
 */
void SPIOutputPort::RunRDMCallback(RDMCallback *callback,
                                   RDMResponse *response) {
  if (!callback) {
    OLA_WARN << "Null callback passed to the SPIOutputPort!";
    return;
  }
  vector<string> packets;
  callback->Run(ola::rdm::RDM_COMPLETED_OK, response, packets);
}


/**
 * Run the RDM callback with an error.
 */
void SPIOutputPort::RunRDMCallback(RDMCallback *callback,
                                   ola::rdm::rdm_response_code code) {
  if (!callback) {
    OLA_WARN << "Null callback passed to the SPIOutputPort!";
    return;
  }
  vector<string> packets;
  callback->Run(code, NULL, packets);
}
}  // spi
}  // plugin
}  // ola
