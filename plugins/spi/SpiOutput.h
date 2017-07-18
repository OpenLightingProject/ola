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
 * SpiOutput.h
 * An RDM-controllable SPI device. Takes up to one universe of DMX.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_SPI_SPIOUTPUT_H_
#define PLUGINS_SPI_SPIOUTPUT_H_

#include <memory>
#include <string>
#include "common/rdm/NetworkManager.h"
#include "ola/DmxBuffer.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/UID.h"
#include "ola/stl/STLUtils.h"
#include "ola/rdm/ResponderOps.h"
#include "ola/rdm/ResponderPersonality.h"
#include "ola/rdm/ResponderSensor.h"

namespace ola {
namespace plugin {
namespace spi {

class SpiOutput: public ola::rdm::DiscoverableRDMControllerInterface {
 public:
  struct Options {
    std::string device_label;
    uint8_t pixel_count;
    uint8_t output_number;

    explicit Options(uint8_t output_number, const std::string &spi_device_name)
        : device_label("SPI Device - " + spi_device_name),
          pixel_count(25),  // For the https://www.adafruit.com/products/738
          output_number(output_number) {
    }
  };

  SpiOutput(const ola::rdm::UID &uid,
            class SpiBackendInterface *backend,
            const Options &options);
  ~SpiOutput();

  std::string GetDeviceLabel() const;
  bool SetDeviceLabel(const std::string &device_label);
  uint8_t GetPersonality() const;
  bool SetPersonality(uint16_t personality);
  uint16_t GetStartAddress() const;
  bool SetStartAddress(uint16_t start_address);
  unsigned int PixelCount() const { return m_pixel_count; }

  std::string Description() const;
  bool WriteDMX(const DmxBuffer &buffer);

  void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
  void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
  void SendRDMRequest(ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *callback);

 private:
  /**
   * The RDM Operations for the MovingLightResponder.
   */
  class RDMOps : public ola::rdm::ResponderOps<SpiOutput> {
   public:
    static RDMOps *Instance() {
      if (!instance)
        instance = new RDMOps();
      return instance;
    }

   private:
    RDMOps() : ola::rdm::ResponderOps<SpiOutput>(PARAM_HANDLERS) {}

    static RDMOps *instance;
  };

  class SpiBackendInterface *m_backend;
  const uint8_t m_output_number;
  std::string m_spi_device_name;
  const ola::rdm::UID m_uid;
  const unsigned int m_pixel_count;
  std::string m_device_label;
  uint16_t m_start_address;  // starts from 1
  bool m_identify_mode;
  std::auto_ptr<ola::rdm::PersonalityCollection> m_personality_collection;
  std::auto_ptr<ola::rdm::PersonalityManager> m_personality_manager;
  ola::rdm::Sensors m_sensors;
  std::auto_ptr<ola::rdm::NetworkManagerInterface> m_network_manager;

  // DMX methods
  bool InternalWriteDMX(const DmxBuffer &buffer);

  void IndividualWS2801Control(const DmxBuffer &buffer);
  void CombinedWS2801Control(const DmxBuffer &buffer);
  void IndividualLPD8806Control(const DmxBuffer &buffer);
  void CombinedLPD8806Control(const DmxBuffer &buffer);
  void IndividualP9813Control(const DmxBuffer &buffer);
  void CombinedP9813Control(const DmxBuffer &buffer);
  void IndividualAPA102Control(const DmxBuffer &buffer);
  void CombinedAPA102Control(const DmxBuffer &buffer);

  unsigned int LPD8806BufferSize() const;
  void WriteSpiData(const uint8_t *data, unsigned int length);

  // RDM methods
  ola::rdm::RDMResponse *GetDeviceInfo(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetProductDetailList(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetDeviceModelDescription(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetManufacturerLabel(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetDeviceLabel(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *SetDeviceLabel(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetSoftwareVersionLabel(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetDmxPersonality(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *SetDmxPersonality(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetPersonalityDescription(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetDmxStartAddress(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *SetDmxStartAddress(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetIdentify(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *SetIdentify(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetSensorDefinition(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetSensorValue(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *SetSensorValue(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *RecordSensor(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetListInterfaces(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetInterfaceLabel(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetInterfaceHardwareAddressType1(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetIPV4CurrentAddress(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetIPV4DefaultRoute(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetDNSHostname(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetDNSDomainName(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetDNSNameServer(
      const ola::rdm::RDMRequest *request);

  // Helpers
  uint8_t P9813CreateFlag(uint8_t red, uint8_t green, uint8_t blue);
  static uint8_t CalculateAPA102LatchBytes(uint16_t pixel_count);

  static const uint8_t SPI_MODE;
  static const uint8_t SPI_BITS_PER_WORD;
  static const uint16_t SPI_DELAY;
  static const uint32_t SPI_SPEED;
  static const uint16_t WS2801_SLOTS_PER_PIXEL;
  static const uint16_t LPD8806_SLOTS_PER_PIXEL;
  static const uint16_t P9813_SLOTS_PER_PIXEL;
  static const uint16_t P9813_SPI_BYTES_PER_PIXEL;
  static const uint16_t APA102_SLOTS_PER_PIXEL;
  static const uint16_t APA102_SPI_BYTES_PER_PIXEL;
  static const uint16_t APA102_START_FRAME_BYTES;

  static const ola::rdm::ResponderOps<SpiOutput>::ParamHandler
      PARAM_HANDLERS[];
};
}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIOUTPUT_H_
