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
 * An OLA SPI Port. This simply wraps the SPIOutput.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_SPI_SPIPORT_H_
#define PLUGINS_SPI_SPIPORT_H_

#include <string>
#include <vector>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/spi/SPIDevice.h"
#include "plugins/spi/SPIOutput.h"

namespace ola {
namespace plugin {
namespace spi {

class SPIOutputPort: public BasicOutputPort {
  public:
    SPIOutputPort(SPIDevice *parent, const string &spi_device,
                  const UID &uid, const SPIOutput::Options &options);
    ~SPIOutputPort() {}

    uint8_t GetPersonality() const;
    bool SetPersonality(uint16_t personality);
    uint16_t GetStartAddress() const;
    bool SetStartAddress(uint16_t start_address);

    string Description() const;
    bool Init();
    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

    void RunFullDiscovery(RDMDiscoveryCallback *callback);
    void RunIncrementalDiscovery(RDMDiscoveryCallback *callback);
    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);

  private:
    SPIOutput m_spi_output;
};
}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIPORT_H_
