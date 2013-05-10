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

#include "plugins/spi/SPIPort.h"

namespace ola {
namespace plugin {
namespace spi {

using ola::rdm::RDMCallback;
using ola::rdm::RDMRequest;
using ola::rdm::UID;

SPIOutputPort::SPIOutputPort(SPIDevice *parent, const string &spi_device,
                             const UID &uid,
                             const SPIBackend::Options &options)
    : BasicOutputPort(parent, 0, true),
      m_spi_backend(spi_device, uid, options) {
}


uint8_t SPIOutputPort::GetPersonality() const {
  return m_spi_backend.GetPersonality();
}

bool SPIOutputPort::SetPersonality(uint16_t personality) {
  return m_spi_backend.SetPersonality(personality);
}

uint16_t SPIOutputPort::GetStartAddress() const {
  return m_spi_backend.GetStartAddress();
}

bool SPIOutputPort::SetStartAddress(uint16_t address) {
  return m_spi_backend.SetStartAddress(address);
}

string SPIOutputPort::Description() const {
  return m_spi_backend.Description();
}

bool SPIOutputPort::Init() {
  return m_spi_backend.Init();
}

bool SPIOutputPort::WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
  return m_spi_backend.WriteDMX(buffer, priority);
}

void SPIOutputPort::RunFullDiscovery(RDMDiscoveryCallback *callback) {
  return m_spi_backend.RunFullDiscovery(callback);
}

void SPIOutputPort::RunIncrementalDiscovery(RDMDiscoveryCallback *callback) {
  return m_spi_backend.RunIncrementalDiscovery(callback);
}

void SPIOutputPort::SendRDMRequest(const ola::rdm::RDMRequest *request,
                                   ola::rdm::RDMCallback *callback) {
  return m_spi_backend.SendRDMRequest(request, callback);
}
}  // namespace spi
}  // namespace plugin
}  // namespace ola
