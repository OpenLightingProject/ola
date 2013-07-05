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
 * SPIBackend.cpp
 * Provides a SPI device which can be managed by RDM.
 * Copyright (C) 2013 Simon Newton
 *
 * The LPD8806 code was based on
 * https://github.com/adafruit/LPD8806/blob/master/LPD8806.cpp
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <sys/ioctl.h>
#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "ola/base/Array.h"
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SocketCloser.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/stl/STLUtils.h"

#include "plugins/spi/SPIBackend.h"

namespace ola {
namespace plugin {
namespace spi {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using ola::rdm::NR_DATA_OUT_OF_RANGE;
using ola::rdm::NR_FORMAT_ERROR;
using ola::rdm::RDMCallback;
using ola::rdm::RDMCommand;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::ResponderHelper;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::auto_ptr;

const uint16_t SPIBackend::SPI_DELAY = 0;
const uint8_t SPIBackend::SPI_BITS_PER_WORD = 8;
const uint8_t SPIBackend::SPI_MODE = 0;
const uint16_t SPIBackend::WS2801_SLOTS_PER_PIXEL = 3;
const uint16_t SPIBackend::LPD8806_SLOTS_PER_PIXEL = 3;

SPIBackend::RDMOps *SPIBackend::RDMOps::instance = NULL;

const ola::rdm::ResponderOps<SPIBackend>::ParamHandler
    SPIBackend::PARAM_HANDLERS[] = {
  { ola::rdm::PID_DEVICE_INFO,
    &SPIBackend::GetDeviceInfo,
    NULL},
  { ola::rdm::PID_PRODUCT_DETAIL_ID_LIST,
    &SPIBackend::GetProductDetailList,
    NULL},
  { ola::rdm::PID_DEVICE_MODEL_DESCRIPTION,
    &SPIBackend::GetDeviceModelDescription,
    NULL},
  { ola::rdm::PID_MANUFACTURER_LABEL,
    &SPIBackend::GetManufacturerLabel,
    NULL},
  { ola::rdm::PID_DEVICE_LABEL,
    &SPIBackend::GetDeviceLabel,
    NULL},
  { ola::rdm::PID_SOFTWARE_VERSION_LABEL,
    &SPIBackend::GetSoftwareVersionLabel,
    NULL},
  { ola::rdm::PID_DMX_PERSONALITY,
    &SPIBackend::GetDmxPersonality,
    &SPIBackend::SetDmxPersonality},
  { ola::rdm::PID_DMX_PERSONALITY_DESCRIPTION,
    &SPIBackend::GetPersonalityDescription,
    NULL},
  { ola::rdm::PID_DMX_START_ADDRESS,
    &SPIBackend::GetDmxStartAddress,
    &SPIBackend::SetDmxStartAddress},
  { ola::rdm::PID_IDENTIFY_DEVICE,
    &SPIBackend::GetIdentify,
    &SPIBackend::SetIdentify},
  { 0, NULL, NULL},
};


SPIBackend::SPIBackend(const string &spi_device,
                       const UID &uid, const Options &options)
    : m_device_path(spi_device),
      m_spi_device_name(spi_device),
      m_uid(uid),
      m_pixel_count(options.pixel_count),
      m_spi_speed(options.spi_speed),
      m_fd(-1),
      m_start_address(1),
      m_identify_mode(false) {
  size_t pos = spi_device.find_last_of("/");
  if (pos != string::npos)
    m_spi_device_name = spi_device.substr(pos + 1);

  m_personality_manager.AddPersonality(m_pixel_count * WS2801_SLOTS_PER_PIXEL,
                                       "WS2801 Individual Control");
  m_personality_manager.AddPersonality(WS2801_SLOTS_PER_PIXEL,
                                       "WS2801 Combined Control");
  m_personality_manager.AddPersonality(m_pixel_count * LPD8806_SLOTS_PER_PIXEL,
                                       "LPD8806 Individual Control");
  m_personality_manager.AddPersonality(LPD8806_SLOTS_PER_PIXEL,
                                       "LPD8806 Combined Control");
  m_personality_manager.SetActivePersonality(1);
}


SPIBackend::~SPIBackend() {
  if (m_fd >= 0)
    close(m_fd);
}

uint8_t SPIBackend::GetPersonality() const {
  return m_personality_manager.ActivePersonalityNumber();
}

bool SPIBackend::SetPersonality(uint16_t personality) {
  return m_personality_manager.SetActivePersonality(personality);
}

uint16_t SPIBackend::GetStartAddress() const {
  return m_start_address;
}

bool SPIBackend::SetStartAddress(uint16_t address) {
  uint16_t footprint = m_personality_manager.ActivePersonalityFootprint();
  uint16_t end_address = DMX_UNIVERSE_SIZE - footprint + 1;
  if (address == 0 || address > end_address || footprint == 0) {
    return false;
  }
  m_start_address = address;
  return true;
}

/**
 * Open the SPI device
 */
bool SPIBackend::Init() {
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

  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &m_spi_speed) < 0) {
    OLA_WARN << "Failed to set SPI_IOC_WR_MAX_SPEED_HZ for " << m_device_path;
    return false;
  }
  m_fd = closer.Release();
  return true;
}


/*
 * Send DMX data over SPI.
 */
bool SPIBackend::WriteDMX(const DmxBuffer &buffer, uint8_t) {
  if (m_fd < 0)
    return false;

  if (m_identify_mode)
    return true;

  switch (m_personality_manager.ActivePersonalityNumber()) {
    case 1:
      IndividualWS2801Control(buffer);
      break;
    case 2:
      CombinedWS2801Control(buffer);
      break;
    case 3:
      IndividualLPD8806Control(buffer);
      break;
    case 4:
      CombinedLPD8806Control(buffer);
      break;
    default:
      break;
  }
  return true;
}


void SPIBackend::RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
  UIDSet uids;
  uids.AddUID(m_uid);
  callback->Run(uids);
}


void SPIBackend::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  UIDSet uids;
  uids.AddUID(m_uid);
  callback->Run(uids);
}


void SPIBackend::SendRDMRequest(const RDMRequest *request,
                                RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ola::rdm::ROOT_RDM_DEVICE,
                                       request, callback);
}

void SPIBackend::IndividualWS2801Control(const DmxBuffer &buffer) {
  unsigned int length = m_pixel_count * WS2801_SLOTS_PER_PIXEL;
  uint8_t output_data[length];
  buffer.GetRange(m_start_address - 1, output_data, &length);
  WriteSPIData(output_data, length);
}

void SPIBackend::CombinedWS2801Control(const DmxBuffer &buffer) {
  unsigned int pixel_data_length = WS2801_SLOTS_PER_PIXEL;
  uint8_t pixel_data[pixel_data_length];
  buffer.GetRange(m_start_address - 1, pixel_data, &pixel_data_length);
  if (pixel_data_length != WS2801_SLOTS_PER_PIXEL) {
    OLA_INFO << "Insufficient DMX data, required " << WS2801_SLOTS_PER_PIXEL
             << ", got " << pixel_data_length;
    return;
  }

  unsigned int length = m_pixel_count * WS2801_SLOTS_PER_PIXEL;
  uint8_t output_data[length];
  for (unsigned int i = 0; i < m_pixel_count; i++) {
    memcpy(output_data + (i * WS2801_SLOTS_PER_PIXEL), pixel_data,
           pixel_data_length);
  }
  WriteSPIData(output_data, length);
}

void SPIBackend::IndividualLPD8806Control(const DmxBuffer &buffer) {
  unsigned int length = LPD8806BufferSize();
  uint8_t output_data[length];
  memset(output_data, 0, length);

  unsigned int first_slot = m_start_address - 1;  // 0 offset
  unsigned int limit = std::min(m_pixel_count * LPD8806_SLOTS_PER_PIXEL,
                                buffer.Size() - first_slot);
  for (unsigned int i = 0; i < limit; i++) {
    uint8_t d = buffer.Get(first_slot + i);
    // Convert RGB to GRB
    switch (i % LPD8806_SLOTS_PER_PIXEL) {
      case 0:
        output_data[i + 1] = 0x80 | (d >> 1);
        break;
      case 1:
        output_data[i - 1] = 0x80 | (d >> 1);
        break;
      default:
        output_data[i] = 0x80 | (d >> 1);
    }
  }
  WriteSPIData(output_data, length);
}

void SPIBackend::CombinedLPD8806Control(const DmxBuffer &buffer) {
  unsigned int pixel_data_length = LPD8806_SLOTS_PER_PIXEL;
  uint8_t pixel_data[pixel_data_length];
  buffer.GetRange(m_start_address - 1, pixel_data, &pixel_data_length);
  if (pixel_data_length != LPD8806_SLOTS_PER_PIXEL) {
    OLA_INFO << "Insufficient DMX data, required " << LPD8806_SLOTS_PER_PIXEL
             << ", got " << pixel_data_length;
    return;
  }

  // The leds are GRB format so convert here
  uint8_t temp = pixel_data[1];
  pixel_data[1] = pixel_data[0];
  pixel_data[0] = temp;

  unsigned int length = LPD8806BufferSize();
  uint8_t output_data[length];
  memset(output_data, 0, length);
  for (unsigned int i = 0; i < m_pixel_count; i++) {
    for (unsigned int j = 0; j < LPD8806_SLOTS_PER_PIXEL; j++) {
      output_data[i * LPD8806_SLOTS_PER_PIXEL + j] =
        0x80 | (pixel_data[j] >> 1);
    }
  }
  WriteSPIData(output_data, length);
}

unsigned int SPIBackend::LPD8806BufferSize() const {
  uint8_t latch_bytes = (m_pixel_count + 31) / 32;
  return m_pixel_count * LPD8806_SLOTS_PER_PIXEL + latch_bytes;
}

void SPIBackend::WriteSPIData(const uint8_t *data, unsigned int length) {
  struct spi_ioc_transfer spi;
  memset(&spi, 0, sizeof(spi));
  spi.tx_buf = reinterpret_cast<__u64>(data);
  spi.len = length;
  int bytes_written = ioctl(m_fd, SPI_IOC_MESSAGE(1), &spi);
  if (bytes_written != static_cast<int>(length)) {
    OLA_WARN << "Failed to write all the SPI data: " << strerror(errno);
  }
}

const RDMResponse *SPIBackend::GetDeviceInfo(const RDMRequest *request) {
  uint16_t footprint = m_personality_manager.ActivePersonalityFootprint();
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }
  return ResponderHelper::GetDeviceInfo(
      request, ola::rdm::OLA_SPI_DEVICE_MODEL,
      ola::rdm::PRODUCT_CATEGORY_FIXTURE, 1,
      footprint,
      m_personality_manager.ActivePersonalityNumber(),
      m_personality_manager.PersonalityCount(),
      footprint ? m_start_address : ola::rdm::ZERO_FOOTPRINT_DMX_ADDRESS,
      0, 0);
}

const RDMResponse *SPIBackend::GetProductDetailList(
    const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(request,
    std::vector<ola::rdm::rdm_product_detail>
        (1, ola::rdm::PRODUCT_DETAIL_LED));
}

const RDMResponse *SPIBackend::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA SPI Device");
}

const RDMResponse *SPIBackend::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(
      request,
      ola::rdm::OLA_MANUFACTURER_LABEL);
}

const RDMResponse *SPIBackend::GetDeviceLabel(const RDMRequest *request) {
  return ResponderHelper::GetString(request, "SPI Device");
}

const RDMResponse *SPIBackend::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

const RDMResponse *SPIBackend::GetDmxPersonality(const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  struct personality_info_s {
    uint8_t personality;
    uint8_t total;
  } __attribute__((packed));

  struct personality_info_s personality_info = {
    m_personality_manager.ActivePersonalityNumber(),
    m_personality_manager.PersonalityCount()
  };
  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&personality_info),
    sizeof(personality_info));
}

const RDMResponse *SPIBackend::SetDmxPersonality(const RDMRequest *request) {
  uint8_t personality_number;
  if (!ResponderHelper::ExtractUInt8(request, &personality_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  const Personality *personality = m_personality_manager.Lookup(
      personality_number);

  if (!personality) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }
  if (m_start_address + personality->footprint() - 1 > DMX_UNIVERSE_SIZE) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  m_personality_manager.SetActivePersonality(personality_number);
  return new ola::rdm::RDMSetResponse(
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

const RDMResponse *SPIBackend::GetPersonalityDescription(
    const RDMRequest *request) {
  uint8_t personality_number;
  if (!ResponderHelper::ExtractUInt8(request, &personality_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  const Personality *personality = m_personality_manager.Lookup(
      personality_number);
  if (!personality) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

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

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&personality_description),
      sizeof(personality_description));
}

const RDMResponse *SPIBackend::GetDmxStartAddress(const RDMRequest *request) {
  return ResponderHelper::GetUInt16Value(
    request,
    ((m_personality_manager.ActivePersonalityFootprint() == 0) ?
     ola::rdm::ZERO_FOOTPRINT_DMX_ADDRESS : m_start_address));
}

const RDMResponse *SPIBackend::SetDmxStartAddress(const RDMRequest *request) {
  uint16_t address;
  if (!ResponderHelper::ExtractUInt16(request, &address)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }
  if (!SetStartAddress(address)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  m_start_address = address;
  return new ola::rdm::RDMSetResponse(
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

const RDMResponse *SPIBackend::GetIdentify(const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_mode);
}

const RDMResponse *SPIBackend::SetIdentify(const RDMRequest *request) {
  bool old_value = m_identify_mode;
  const RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_mode);
  if (m_identify_mode != old_value) {
    OLA_INFO << "SPI " << m_spi_device_name << " identify mode " << (
        m_identify_mode ? "on" : "off");
    DmxBuffer identify_buffer;
    if (m_identify_mode)
      identify_buffer.SetRangeToValue(0, DMX_MAX_CHANNEL_VALUE,
                                      DMX_UNIVERSE_SIZE);
    else
      identify_buffer.Blackout();
  }
  return response;
}
}  // namespace spi
}  // namespace plugin
}  // namespace ola
