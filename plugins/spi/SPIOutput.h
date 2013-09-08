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

#include <string>
#include <vector>
#include "ola/DmxBuffer.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/UID.h"
#include "ola/stl/STLUtils.h"
#include "ola/rdm/ResponderOps.h"

namespace ola {
namespace plugin {
namespace spi {

using ola::rdm::UID;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;

class Personality {
  public:
    Personality(uint16_t footprint, const string &description)
        : m_footprint(footprint),
          m_description(description) {
    }

    uint16_t footprint() const { return m_footprint; }
    string description() const { return m_description; }

  private:
    uint16_t m_footprint;
    const string m_description;
};

class PersonalityManager {
  public:
    PersonalityManager() : m_active_personality(0) {}

    ~PersonalityManager() {
      STLDeleteElements(&m_personalities);
    }

    void AddPersonality(uint8_t footprint, const string &description) {
      m_personalities.push_back(new Personality(footprint, description));
    }

    uint8_t PersonalityCount() const { return m_personalities.size(); }

    bool SetActivePersonality(uint8_t personality) {
      if (personality == 0 || personality > m_personalities.size())
        return false;
      m_active_personality = personality;
      return true;
    }

    uint8_t ActivePersonalityNumber() const { return m_active_personality; }

    const Personality *ActivePersonality() const {
      return Lookup(m_active_personality);
    }

    uint16_t ActivePersonalityFootprint() const {
      const Personality *personality = Lookup(m_active_personality);
      return personality ? personality->footprint() : 0;
    }

    string ActivePersonalityDescription() const {
      const Personality *personality = Lookup(m_active_personality);
      return personality ? personality->description() : "";
    }

    // Lookup a personality. Personalities are numbers from 1.
    const Personality *Lookup(uint8_t personality) const {
      if (personality == 0 || personality > m_personalities.size())
        return NULL;
      return m_personalities[personality - 1];
    }

  private:
    std::vector<Personality*> m_personalities;
    uint8_t m_active_personality;
};


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
              class SPIBackend *backend,
              const Options &options);

    uint8_t GetPersonality() const;
    bool SetPersonality(uint16_t personality);
    uint16_t GetStartAddress() const;
    bool SetStartAddress(uint16_t start_address);

    string Description() const;
    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

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

    class SPIBackend *m_backend;
    const uint8_t m_output_number;
    string m_spi_device_name;
    const UID m_uid;
    const unsigned int m_pixel_count;
    uint16_t m_start_address;  // starts from 1
    bool m_identify_mode;
    PersonalityManager m_personality_manager;

    // DMX methods
    void IndividualWS2801Control(const DmxBuffer &buffer);
    void CombinedWS2801Control(const DmxBuffer &buffer);
    void IndividualLPD8806Control(const DmxBuffer &buffer);
    void CombinedLPD8806Control(const DmxBuffer &buffer);
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

    static const uint8_t SPI_MODE;
    static const uint8_t SPI_BITS_PER_WORD;
    static const uint16_t SPI_DELAY;
    static const uint32_t SPI_SPEED;
    static const uint16_t WS2801_SLOTS_PER_PIXEL;
    static const uint16_t LPD8806_SLOTS_PER_PIXEL;

    static const ola::rdm::ResponderOps<SPIOutput>::ParamHandler
        PARAM_HANDLERS[];
};
}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIOUTPUT_H_
