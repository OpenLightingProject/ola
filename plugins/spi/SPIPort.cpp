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

#include <string>
#include "ola/BaseTypes.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/UID.h"

#include "plugins/spi/SPIBackend.h"
#include "plugins/spi/SPIPort.h"

namespace ola {
namespace plugin {
namespace spi {

using ola::rdm::RDMCallback;
using ola::rdm::RDMRequest;
using ola::rdm::UID;

SPIOutputPort::SPIOutputPort(SPIDevice *parent, SPIBackendInterface *backend,
                             const UID &uid,
                             const SPIOutput::Options &options)
    : BasicOutputPort(parent, options.output_number, true),
      m_spi_output(uid, backend, options) {
}


uint8_t SPIOutputPort::GetPersonality() const {
  return m_spi_output.GetPersonality();
}

bool SPIOutputPort::SetPersonality(uint16_t personality) {
  return m_spi_output.SetPersonality(personality);
}

uint16_t SPIOutputPort::GetStartAddress() const {
  return m_spi_output.GetStartAddress();
}

bool SPIOutputPort::SetStartAddress(uint16_t address) {
  return m_spi_output.SetStartAddress(address);
}

string SPIOutputPort::Description() const {
  return m_spi_output.Description();
}

bool SPIOutputPort::WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
  return m_spi_output.WriteDMX(buffer, priority);
}

void SPIOutputPort::RunFullDiscovery(RDMDiscoveryCallback *callback) {
  return m_spi_output.RunFullDiscovery(callback);
}

void SPIOutputPort::RunIncrementalDiscovery(RDMDiscoveryCallback *callback) {
  return m_spi_output.RunIncrementalDiscovery(callback);
}

void SPIOutputPort::SendRDMRequest(const ola::rdm::RDMRequest *request,
                                   ola::rdm::RDMCallback *callback) {
  return m_spi_output.SendRDMRequest(request, callback);
}
}  // namespace spi
}  // namespace plugin
}  // namespace ola
