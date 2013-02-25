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

#ifndef PLUGINS_SPI_SPIPORT_H_
#define PLUGINS_SPI_SPIPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/spi/SPIDevice.h"

namespace ola {
namespace plugin {
namespace spi {

class SPIOutputPort: public BasicOutputPort {
  public:
    SPIOutputPort(SPIDevice *parent, const string &spi_device,
                  const UID &uid, uint8_t pixel_count);
    ~SPIOutputPort();

    string Description() const { return m_spi_device_name; }
    bool Init();
    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

    void RunFullDiscovery(RDMDiscoveryCallback *callback);
    void RunIncrementalDiscovery(RDMDiscoveryCallback *callback);
    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);

  private:

    const string m_device_path;
    string m_spi_device_name;
    const UID m_uid;
    const unsigned int m_pixel_count;
    int m_fd;
    uint8_t m_personality;
    uint16_t m_start_address;  // starts from 1
    bool m_identify_mode;

    uint16_t Footprint() const;
    uint16_t PersonalityFootprint(uint8_t personality) const;
    string PersonalityDescription(uint8_t personality) const;
    void HandleUnknownPacket(const ola::rdm::RDMRequest *request,
                             ola::rdm::RDMCallback *callback);
    void HandleSupportedParams(const ola::rdm::RDMRequest *request,
                               ola::rdm::RDMCallback *callback);
    void HandleDeviceInfo(const ola::rdm::RDMRequest *request,
                          ola::rdm::RDMCallback *callback);
    void HandleProductDetailList(const ola::rdm::RDMRequest *request,
                                 ola::rdm::RDMCallback *callback);
    void HandleStringResponse(const ola::rdm::RDMRequest *request,
                              ola::rdm::RDMCallback *callback,
                              const std::string &value);
    void HandlePersonality(const ola::rdm::RDMRequest *request,
                           ola::rdm::RDMCallback *callback);
    void HandlePersonalityDescription(const ola::rdm::RDMRequest *request,
                                      ola::rdm::RDMCallback *callback);
    void HandleDmxStartAddress(const ola::rdm::RDMRequest *request,
                               ola::rdm::RDMCallback *callback);
    void HandleIdentifyDevice(const ola::rdm::RDMRequest *request,
                              ola::rdm::RDMCallback *callback);
    bool CheckForBroadcastSubdeviceOrData(const ola::rdm::RDMRequest *request,
                                          ola::rdm::RDMCallback *callback);
    void RunRDMCallback(ola::rdm::RDMCallback *callback,
                        ola::rdm::RDMResponse *response);
    void RunRDMCallback(ola::rdm::RDMCallback *callback,
                        ola::rdm::rdm_response_code code);

    static const uint8_t SPI_MODE;
    static const uint8_t SPI_BITS_PER_WORD;
    static const uint16_t SPI_DELAY;
    static const uint32_t SPI_SPEED;
    static const uint8_t PERSONALITY_WS2801_INDIVIDUAL;
    static const uint8_t PERSONALITY_WS2801_SIMULATANEOUS;
    static const uint8_t PERSONALITY_LAST;
    static const uint16_t CHANNELS_PER_PIXEL;
};
}  // spi
}  // plugin
}  // ola
#endif  // PLUGINS_SPI_SPIPORT_H_
