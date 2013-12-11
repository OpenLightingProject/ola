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
 * SPIOutput.cpp
 * An RDM-controllable SPI device. Takes up to one universe of DMX.
 * Copyright (C) 2013 Simon Newton
 *
 * The LPD8806 code was based on
 * https://github.com/adafruit/LPD8806/blob/master/LPD8806.cpp
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "ola/base/Array.h"
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/rdm/ResponderLoadSensor.h"
#include "ola/rdm/ResponderSensor.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/stl/STLUtils.h"

#include "plugins/spi/SPIBackend.h"
#include "plugins/spi/SPIOutput.h"

namespace ola {
namespace plugin {
namespace spi {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using ola::rdm::LoadSensor;
using ola::rdm::NR_DATA_OUT_OF_RANGE;
using ola::rdm::NR_FORMAT_ERROR;
using ola::rdm::Personality;
using ola::rdm::PersonalityCollection;
using ola::rdm::PersonalityManager;
using ola::rdm::RDMCallback;
using ola::rdm::RDMCommand;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::ResponderHelper;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::auto_ptr;
using std::min;

const uint16_t SPIOutput::SPI_DELAY = 0;
const uint8_t SPIOutput::SPI_BITS_PER_WORD = 8;
const uint8_t SPIOutput::SPI_MODE = 0;

/**
 * These constants are used to determine the number of DMX Slots per pixel
 * The p9813 uses another byte preceding each of the three bytes as a kind
 * of header.
 */
const uint16_t SPIOutput::WS2801_SLOTS_PER_PIXEL = 3;
const uint16_t SPIOutput::LPD8806_SLOTS_PER_PIXEL = 3;
const uint16_t SPIOutput::P9813_SLOTS_PER_PIXEL = 3;

// Number of bytes that each P9813 pixel uses on the spi wires
const uint16_t SPIOutput::P9813_SPI_BYTES_PER_PIXEL = 4;

SPIOutput::RDMOps *SPIOutput::RDMOps::instance = NULL;

const ola::rdm::ResponderOps<SPIOutput>::ParamHandler
    SPIOutput::PARAM_HANDLERS[] = {
  { ola::rdm::PID_DEVICE_INFO,
    &SPIOutput::GetDeviceInfo,
    NULL},
  { ola::rdm::PID_PRODUCT_DETAIL_ID_LIST,
    &SPIOutput::GetProductDetailList,
    NULL},
  { ola::rdm::PID_DEVICE_MODEL_DESCRIPTION,
    &SPIOutput::GetDeviceModelDescription,
    NULL},
  { ola::rdm::PID_MANUFACTURER_LABEL,
    &SPIOutput::GetManufacturerLabel,
    NULL},
  { ola::rdm::PID_DEVICE_LABEL,
    &SPIOutput::GetDeviceLabel,
    NULL},
  { ola::rdm::PID_SOFTWARE_VERSION_LABEL,
    &SPIOutput::GetSoftwareVersionLabel,
    NULL},
  { ola::rdm::PID_DMX_PERSONALITY,
    &SPIOutput::GetDmxPersonality,
    &SPIOutput::SetDmxPersonality},
  { ola::rdm::PID_DMX_PERSONALITY_DESCRIPTION,
    &SPIOutput::GetPersonalityDescription,
    NULL},
  { ola::rdm::PID_DMX_START_ADDRESS,
    &SPIOutput::GetDmxStartAddress,
    &SPIOutput::SetDmxStartAddress},
  { ola::rdm::PID_IDENTIFY_DEVICE,
    &SPIOutput::GetIdentify,
    &SPIOutput::SetIdentify},
#ifdef HAVE_GETLOADAVG
  { ola::rdm::PID_SENSOR_DEFINITION,
    &SPIOutput::GetSensorDefinition,
    NULL},
  { ola::rdm::PID_SENSOR_VALUE,
    &SPIOutput::GetSensorValue,
    &SPIOutput::SetSensorValue},
  { ola::rdm::PID_RECORD_SENSORS,
    NULL,
    &SPIOutput::RecordSensor},
#endif
  { 0, NULL, NULL},
};


SPIOutput::SPIOutput(const UID &uid, SPIBackendInterface *backend,
                     const Options &options)
    : m_backend(backend),
      m_output_number(options.output_number),
      m_uid(uid),
      m_pixel_count(options.pixel_count),
      m_start_address(1),
      m_identify_mode(false) {
  const string device_path(m_backend->DevicePath());
  size_t pos = device_path.find_last_of("/");
  if (pos != string::npos)
    m_spi_device_name = device_path.substr(pos + 1);

  PersonalityCollection::PersonalityList personalities;
  personalities.push_back(Personality(m_pixel_count * WS2801_SLOTS_PER_PIXEL,
                                      "WS2801 Individual Control"));
  personalities.push_back(Personality(WS2801_SLOTS_PER_PIXEL,
                                      "WS2801 Combined Control"));
  personalities.push_back(Personality(m_pixel_count * LPD8806_SLOTS_PER_PIXEL,
                                      "LPD8806 Individual Control"));
  personalities.push_back(Personality(LPD8806_SLOTS_PER_PIXEL,
                                      "LPD8806 Combined Control"));
  personalities.push_back(Personality(m_pixel_count * P9813_SLOTS_PER_PIXEL,
                                      "P9813 Individual Control"));
  personalities.push_back(Personality(P9813_SLOTS_PER_PIXEL,
                                      "P9813 Combined Control"));
  m_personality_manager = PersonalityManager(
      new PersonalityCollection(personalities));
  m_personality_manager.SetActivePersonality(1);

#ifdef HAVE_GETLOADAVG
  m_sensors.push_back(new LoadSensor(ola::system::LOAD_AVERAGE_1_MIN,
                                     "Load Average 1 minute"));
  m_sensors.push_back(new LoadSensor(ola::system::LOAD_AVERAGE_5_MINS,
                                     "Load Average 5 minutes"));
  m_sensors.push_back(new LoadSensor(ola::system::LOAD_AVERAGE_15_MINS,
                                     "Load Average 15 minutes"));
#endif
}


uint8_t SPIOutput::GetPersonality() const {
  return m_personality_manager.ActivePersonalityNumber();
}

bool SPIOutput::SetPersonality(uint16_t personality) {
  return m_personality_manager.SetActivePersonality(personality);
}

uint16_t SPIOutput::GetStartAddress() const {
  return m_start_address;
}

bool SPIOutput::SetStartAddress(uint16_t address) {
  uint16_t footprint = m_personality_manager.ActivePersonalityFootprint();
  uint16_t end_address = DMX_UNIVERSE_SIZE - footprint + 1;
  if (address == 0 || address > end_address || footprint == 0) {
    return false;
  }
  m_start_address = address;
  return true;
}

string SPIOutput::Description() const {
  std::ostringstream str;
  str << m_spi_device_name << ", output "
      << static_cast<int>(m_output_number) << ", "
      << m_personality_manager.ActivePersonalityDescription() << ", "
      << m_personality_manager.ActivePersonalityFootprint()
      << " slots @ " << m_start_address << ". (" << m_uid << ")";
  return str.str();
}

/*
 * Send DMX data over SPI.
 */
bool SPIOutput::WriteDMX(const DmxBuffer &buffer) {
  if (m_identify_mode)
    return true;
  return InternalWriteDMX(buffer);
}


void SPIOutput::RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
  UIDSet uids;
  uids.AddUID(m_uid);
  callback->Run(uids);
}


void SPIOutput::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  UIDSet uids;
  uids.AddUID(m_uid);
  callback->Run(uids);
}


void SPIOutput::SendRDMRequest(const RDMRequest *request,
                                RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ola::rdm::ROOT_RDM_DEVICE,
                                       request, callback);
}

bool SPIOutput::InternalWriteDMX(const DmxBuffer &buffer) {
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
    case 5:
      IndividualP9813Control(buffer);
      break;
    case 6:
      CombinedP9813Control(buffer);
      break;
    default:
      break;
  }
  return true;
}

void SPIOutput::IndividualWS2801Control(const DmxBuffer &buffer) {
  // We always check out the entire string length, even if we only have data
  // for part of it
  const unsigned int output_length = m_pixel_count * LPD8806_SLOTS_PER_PIXEL;
  uint8_t *output = m_backend->Checkout(m_output_number, output_length);
  if (!output)
    return;

  unsigned int new_length = output_length;
  buffer.GetRange(m_start_address - 1, output, &new_length);
  m_backend->Commit(m_output_number);
}

void SPIOutput::CombinedWS2801Control(const DmxBuffer &buffer) {
  unsigned int pixel_data_length = WS2801_SLOTS_PER_PIXEL;
  uint8_t pixel_data[WS2801_SLOTS_PER_PIXEL];
  buffer.GetRange(m_start_address - 1, pixel_data, &pixel_data_length);
  if (pixel_data_length != WS2801_SLOTS_PER_PIXEL) {
    OLA_INFO << "Insufficient DMX data, required " << WS2801_SLOTS_PER_PIXEL
             << ", got " << pixel_data_length;
    return;
  }

  const unsigned int length = m_pixel_count * WS2801_SLOTS_PER_PIXEL;
  uint8_t *output = m_backend->Checkout(m_output_number, length);
  if (!output)
    return;

  for (unsigned int i = 0; i < m_pixel_count; i++) {
    memcpy(output + (i * WS2801_SLOTS_PER_PIXEL), pixel_data,
           pixel_data_length);
  }
  m_backend->Commit(m_output_number);
}

void SPIOutput::IndividualLPD8806Control(const DmxBuffer &buffer) {
  const uint8_t latch_bytes = (m_pixel_count + 31) / 32;
  const unsigned int first_slot = m_start_address - 1;  // 0 offset
  if (buffer.Size() - first_slot < LPD8806_SLOTS_PER_PIXEL) {
    // not even 3 bytes of data, don't bother updating
    return;
  }

  // We always check out the entire string length, even if we only have data
  // for part of it
  const unsigned int output_length = m_pixel_count * LPD8806_SLOTS_PER_PIXEL;
  uint8_t *output = m_backend->Checkout(m_output_number, output_length,
                                        latch_bytes);
  if (!output)
    return;

  const unsigned int length = std::min(m_pixel_count * LPD8806_SLOTS_PER_PIXEL,
                                       buffer.Size() - first_slot);

  for (unsigned int i = 0; i < length / LPD8806_SLOTS_PER_PIXEL; i++) {
    // Convert RGB to GRB
    unsigned int offset = first_slot + i * LPD8806_SLOTS_PER_PIXEL;
    uint8_t r = buffer.Get(offset);
    uint8_t g = buffer.Get(offset + 1);
    uint8_t b = buffer.Get(offset + 2);
    output[i * LPD8806_SLOTS_PER_PIXEL] = 0x80 | (g >> 1);
    output[i * LPD8806_SLOTS_PER_PIXEL + 1] = 0x80 | (r >> 1);
    output[i * LPD8806_SLOTS_PER_PIXEL + 2] = 0x80 | (b >> 1);
  }
  m_backend->Commit(m_output_number);
}

void SPIOutput::CombinedLPD8806Control(const DmxBuffer &buffer) {
  const uint8_t latch_bytes = (m_pixel_count + 31) / 32;
  unsigned int pixel_data_length = LPD8806_SLOTS_PER_PIXEL;

  uint8_t pixel_data[LPD8806_SLOTS_PER_PIXEL];
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

  const unsigned int length = m_pixel_count * LPD8806_SLOTS_PER_PIXEL;
  uint8_t *output = m_backend->Checkout(m_output_number, length, latch_bytes);
  if (!output)
    return;

  for (unsigned int i = 0; i < m_pixel_count; i++) {
    for (unsigned int j = 0; j < LPD8806_SLOTS_PER_PIXEL; j++) {
      output[i * LPD8806_SLOTS_PER_PIXEL + j] = 0x80 | (pixel_data[j] >> 1);
    }
  }
  m_backend->Commit(m_output_number);
}

void SPIOutput::IndividualP9813Control(const DmxBuffer &buffer) {
  // We need 4 bytes of zeros in the beginning and 8 bytes at
  // the end
  const uint8_t latch_bytes = 3 * P9813_SPI_BYTES_PER_PIXEL;
  const unsigned int first_slot = m_start_address - 1;  // 0 offset
  if (buffer.Size() - first_slot < P9813_SLOTS_PER_PIXEL) {
    // not even 3 bytes of data, don't bother updating
    return;
  }

  // We always check out the entire string length, even if we only have data
  // for part of it
  const unsigned int output_length = m_pixel_count * P9813_SPI_BYTES_PER_PIXEL;
  uint8_t *output = m_backend->Checkout(m_output_number, output_length,
                                        latch_bytes);

  if (!output)
    return;

  for (unsigned int i = 0; i < m_pixel_count; i++) {
    // Convert RGB to P9813 Pixel
    unsigned int offset = first_slot + i * P9813_SLOTS_PER_PIXEL;
    // We need to avoid the first 4 bytes of the buffer since that acts as a
    // start of frame delimiter
    unsigned int spi_offset = (i + 1) * P9813_SPI_BYTES_PER_PIXEL;
    uint8_t r = 0;
    uint8_t b = 0;
    uint8_t g = 0;
    if (buffer.Size() - offset >= P9813_SLOTS_PER_PIXEL) {
      r = buffer.Get(offset);
      g = buffer.Get(offset + 1);
      b = buffer.Get(offset + 2);
    }
    output[spi_offset] = P9813CreateFlag(r, g, b);
    output[spi_offset + 1] = b;
    output[spi_offset + 2] = g;
    output[spi_offset + 3] = r;
  }
  m_backend->Commit(m_output_number);
}

void SPIOutput::CombinedP9813Control(const DmxBuffer &buffer) {
  const uint8_t latch_bytes = 3 * P9813_SPI_BYTES_PER_PIXEL;
  const unsigned int first_slot = m_start_address - 1;  // 0 offset

  if (buffer.Size() - first_slot < P9813_SLOTS_PER_PIXEL) {
    OLA_INFO << "Insufficient DMX data, required " << P9813_SLOTS_PER_PIXEL
             << ", got " << buffer.Size() - first_slot;
    return;
  }

  uint8_t pixel_data[P9813_SPI_BYTES_PER_PIXEL];
  pixel_data[3] = buffer.Get(first_slot);  // Get Red
  pixel_data[2] = buffer.Get(first_slot + 1);  // Get Green
  pixel_data[1] = buffer.Get(first_slot + 2);  // Get Blue
  pixel_data[0] = P9813CreateFlag(pixel_data[3], pixel_data[2],
                                  pixel_data[1]);

  const unsigned int length = m_pixel_count * P9813_SPI_BYTES_PER_PIXEL;
  uint8_t *output = m_backend->Checkout(m_output_number, length, latch_bytes);
  if (!output)
    return;

  for (unsigned int i = 0; i < m_pixel_count; i++) {
    memcpy(&output[(i + 1) * P9813_SPI_BYTES_PER_PIXEL], pixel_data,
           P9813_SPI_BYTES_PER_PIXEL);
  }
  m_backend->Commit(m_output_number);
}

/**
 * For more information please visit:
 * https://github.com/CoolNeon/elinux-tcl/blob/master/README.txt
 */
uint8_t SPIOutput::P9813CreateFlag(uint8_t red, uint8_t green, uint8_t blue) {
  uint8_t flag = 0;
  flag =  (red & 0xc0) >> 6;
  flag |= (green & 0xc0) >> 4;
  flag |= (blue & 0xc0) >> 2;
  return ~flag;
}

const RDMResponse *SPIOutput::GetDeviceInfo(const RDMRequest *request) {
  return ResponderHelper::GetDeviceInfo(
      request, ola::rdm::OLA_SPI_DEVICE_MODEL,
      ola::rdm::PRODUCT_CATEGORY_FIXTURE, 2,
      &m_personality_manager,
      m_start_address,
      0, m_sensors.size());
}

const RDMResponse *SPIOutput::GetProductDetailList(
    const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(request,
    std::vector<ola::rdm::rdm_product_detail>
        (1, ola::rdm::PRODUCT_DETAIL_LED));
}

const RDMResponse *SPIOutput::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA SPI Device");
}

const RDMResponse *SPIOutput::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(
      request,
      ola::rdm::OLA_MANUFACTURER_LABEL);
}

const RDMResponse *SPIOutput::GetDeviceLabel(const RDMRequest *request) {
  return ResponderHelper::GetString(request, "SPI Device");
}

const RDMResponse *SPIOutput::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

const RDMResponse *SPIOutput::GetDmxPersonality(const RDMRequest *request) {
  return ResponderHelper::GetPersonality(request, &m_personality_manager);
}

const RDMResponse *SPIOutput::SetDmxPersonality(const RDMRequest *request) {
  return ResponderHelper::SetPersonality(request, &m_personality_manager,
                                         m_start_address);
}

const RDMResponse *SPIOutput::GetPersonalityDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonalityDescription(
      request, &m_personality_manager);
}

const RDMResponse *SPIOutput::GetDmxStartAddress(const RDMRequest *request) {
  return ResponderHelper::GetDmxAddress(request, &m_personality_manager,
                                        m_start_address);
}

const RDMResponse *SPIOutput::SetDmxStartAddress(const RDMRequest *request) {
  return ResponderHelper::SetDmxAddress(request, &m_personality_manager,
                                        &m_start_address);
}

const RDMResponse *SPIOutput::GetIdentify(const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_mode);
}

const RDMResponse *SPIOutput::SetIdentify(const RDMRequest *request) {
  bool old_value = m_identify_mode;
  const RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_mode);
  if (m_identify_mode != old_value) {
    OLA_INFO << "SPI " << m_spi_device_name << " identify mode " << (
        m_identify_mode ? "on" : "off");
    DmxBuffer identify_buffer;
    if (m_identify_mode) {
      identify_buffer.SetRangeToValue(0, DMX_MAX_CHANNEL_VALUE,
                                      DMX_UNIVERSE_SIZE);
    } else {
      identify_buffer.Blackout();
    }
    InternalWriteDMX(identify_buffer);
  }
  return response;
}

/**
 * PID_SENSOR_DEFINITION
 */
const RDMResponse *SPIOutput::GetSensorDefinition(
    const RDMRequest *request) {
  return ResponderHelper::GetSensorDefinition(request, m_sensors);
}

/**
 * PID_SENSOR_VALUE
 */
const RDMResponse *SPIOutput::GetSensorValue(const RDMRequest *request) {
  return ResponderHelper::GetSensorValue(request, m_sensors);
}

const RDMResponse *SPIOutput::SetSensorValue(const RDMRequest *request) {
  return ResponderHelper::SetSensorValue(request, m_sensors);
}

/**
 * PID_RECORD_SENSORS
 */
const RDMResponse *SPIOutput::RecordSensor(const RDMRequest *request) {
  return ResponderHelper::RecordSensor(request, m_sensors);
}
}  // namespace spi
}  // namespace plugin
}  // namespace ola
