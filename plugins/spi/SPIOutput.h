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
 * SPIOutput.h
 * An RDM-controllable SPI device. Takes up to one universe of DMX.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_SPI_SPIOUTPUT_H_
#define PLUGINS_SPI_SPIOUTPUT_H_

#include <memory>
#include <string>
#include <vector>
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

using ola::rdm::UID;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;

class SPIOutput: public ola::rdm::DiscoverableRDMControllerInterface {
 public:
  struct Options {
    uint8_t pixel_count;
    uint8_t output_number;

    explicit Options(uint8_t output_number)
        : pixel_count(25),  // For the https://www.adafruit.com/products/738
          output_number(output_number) {
    }
  };

  SPIOutput(const UID &uid,
            class SPIBackendInterface *backend,
            const Options &options);
  ~SPIOutput();

  uint8_t GetPersonality() const;
  bool SetPersonality(uint16_t personality);
  uint16_t GetStartAddress() const;
  bool SetStartAddress(uint16_t start_address);
  unsigned int PixelCount() const { return m_pixel_count; }

  string Description() const;
  bool WriteDMX(const DmxBuffer &buffer);

  void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
  void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
  void SendRDMRequest(const ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *callback);

 private:
  /**
   * The RDM Operations for the MovingLightResponder.
   */
  class RDMOps : public ola::rdm::ResponderOps<SPIOutput> {
   public:
    static RDMOps *Instance() {
      if (!instance)
        instance = new RDMOps();
      return instance;
    }

   private:
    RDMOps() : ola::rdm::ResponderOps<SPIOutput>(PARAM_HANDLERS) {}

    static RDMOps *instance;
  };

  class SPIBackendInterface *m_backend;
  const uint8_t m_output_number;
  string m_spi_device_name;
  const UID m_uid;
  const unsigned int m_pixel_count;
  uint16_t m_start_address;  // starts from 1
  bool m_identify_mode;
  std::auto_ptr<ola::rdm::PersonalityCollection> m_personality_collection;
  std::auto_ptr<ola::rdm::PersonalityManager> m_personality_manager;
  ola::rdm::Sensors m_sensors;

  // DMX methods
  bool InternalWriteDMX(const DmxBuffer &buffer);
  void IndividualWS2801Control(const DmxBuffer &buffer);
  void CombinedWS2801Control(const DmxBuffer &buffer);
  void IndividualLPD8806Control(const DmxBuffer &buffer);
  void CombinedLPD8806Control(const DmxBuffer &buffer);
  void IndividualP9813Control(const DmxBuffer &buffer);
  void CombinedP9813Control(const DmxBuffer &buffer);
  unsigned int LPD8806BufferSize() const;
  void WriteSPIData(const uint8_t *data, unsigned int length);

  // RDM methods
  const RDMResponse *GetDeviceInfo(const RDMRequest *request);
  const RDMResponse *GetProductDetailList(const RDMRequest *request);
  const RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
  const RDMResponse *GetManufacturerLabel(const RDMRequest *request);
  const RDMResponse *GetDeviceLabel(const RDMRequest *request);
  const RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);
  const RDMResponse *GetDmxPersonality(const RDMRequest *request);
  const RDMResponse *SetDmxPersonality(const RDMRequest *request);
  const RDMResponse *GetPersonalityDescription(const RDMRequest *request);
  const RDMResponse *GetDmxStartAddress(const RDMRequest *request);
  const RDMResponse *SetDmxStartAddress(const RDMRequest *request);
  const RDMResponse *GetIdentify(const RDMRequest *request);
  const RDMResponse *SetIdentify(const RDMRequest *request);
  const RDMResponse *GetSensorDefinition(const RDMRequest *request);
  const RDMResponse *GetSensorValue(const RDMRequest *request);
  const RDMResponse *SetSensorValue(const RDMRequest *request);
  const RDMResponse *RecordSensor(const RDMRequest *request);


  // Helpers
  uint8_t P9813CreateFlag(uint8_t red, uint8_t green, uint8_t blue);

  static const uint8_t SPI_MODE;
  static const uint8_t SPI_BITS_PER_WORD;
  static const uint16_t SPI_DELAY;
  static const uint32_t SPI_SPEED;
  static const uint16_t WS2801_SLOTS_PER_PIXEL;
  static const uint16_t LPD8806_SLOTS_PER_PIXEL;
  static const uint16_t P9813_SLOTS_PER_PIXEL;
  static const uint16_t P9813_SPI_BYTES_PER_PIXEL;

  static const ola::rdm::ResponderOps<SPIOutput>::ParamHandler
      PARAM_HANDLERS[];
};
}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIOUTPUT_H_
