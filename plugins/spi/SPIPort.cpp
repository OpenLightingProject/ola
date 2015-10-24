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
 * SPIPort.cpp
 * The SPI plugin for ola
 * Copyright (C) 2013 Simon Newton
 */

#include <string>
#include "ola/Constants.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/UID.h"

#include "plugins/spi/SPIBackend.h"
#include "plugins/spi/SPIPort.h"

namespace ola {
namespace plugin {
namespace spi {

using ola::rdm::RDMCallback;
using ola::rdm::RDMDiscoveryCallback;
using ola::rdm::RDMRequest;
using ola::rdm::UID;
using std::string;

SPIOutputPort::SPIOutputPort(SPIDevice *parent, SPIBackendInterface *backend,
                             const UID &uid,
                             const SPIOutput::Options &options)
    : BasicOutputPort(parent, options.output_number, true),
      m_spi_output(uid, backend, options) {
}


string SPIOutputPort::GetDeviceLabel() const {
  return m_spi_output.GetDeviceLabel();
}

bool SPIOutputPort::SetDeviceLabel(const string &device_label) {
  return m_spi_output.SetDeviceLabel(device_label);
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

unsigned int SPIOutputPort::PixelCount() const {
  return m_spi_output.PixelCount();
}

string SPIOutputPort::Description() const {
  return m_spi_output.Description();
}

bool SPIOutputPort::WriteDMX(const DmxBuffer &buffer, uint8_t) {
  return m_spi_output.WriteDMX(buffer);
}

void SPIOutputPort::RunFullDiscovery(RDMDiscoveryCallback *callback) {
  return m_spi_output.RunFullDiscovery(callback);
}

void SPIOutputPort::RunIncrementalDiscovery(RDMDiscoveryCallback *callback) {
  return m_spi_output.RunIncrementalDiscovery(callback);
}

void SPIOutputPort::SendRDMRequest(ola::rdm::RDMRequest *request,
                                   ola::rdm::RDMCallback *callback) {
  return m_spi_output.SendRDMRequest(request, callback);
}
}  // namespace spi
}  // namespace plugin
}  // namespace ola
