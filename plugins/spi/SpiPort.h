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
 * SpiPort.h
 * An OLA SPI Port. This simply wraps the SpiOutput.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_SPI_SPIPORT_H_
#define PLUGINS_SPI_SPIPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/spi/SpiDevice.h"
#include "plugins/spi/SpiOutput.h"

namespace ola {
namespace plugin {
namespace spi {

class SpiOutputPort: public BasicOutputPort {
 public:
  SpiOutputPort(SpiDevice *parent, class SpiBackendInterface *backend,
                const ola::rdm::UID &uid, const SpiOutput::Options &options);
  ~SpiOutputPort() {}

  std::string GetDeviceLabel() const;
  bool SetDeviceLabel(const std::string &device_label);
  uint8_t GetPersonality() const;
  bool SetPersonality(uint16_t personality);
  uint16_t GetStartAddress() const;
  bool SetStartAddress(uint16_t start_address);
  unsigned int PixelCount() const;

  std::string Description() const;
  bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

  void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
  void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
  void SendRDMRequest(ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *callback);

 private:
  SpiOutput m_spi_output;
};
}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIPORT_H_
