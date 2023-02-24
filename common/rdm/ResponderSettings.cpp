/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * ResponderSettings.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include "ola/rdm/ResponderSettings.h"

#include <algorithm>
#include <string>

#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/strings/Utils.h"

namespace ola {
namespace rdm {

using std::string;

BasicSetting::BasicSetting(const ArgType description)
    : m_description(description) {
}

unsigned int BasicSetting::GenerateDescriptionResponse(uint8_t index,
                                                       uint8_t *data) const {
  description_s *output = reinterpret_cast<description_s*>(data);
  output->setting = index;
  strings::CopyToFixedLengthBuffer(m_description, output->description,
                                   arraysize(output->description));
  return (sizeof(description_s) - MAX_RDM_STRING_LENGTH +
          std::min(static_cast<size_t>(MAX_RDM_STRING_LENGTH),
                   m_description.size()));
}

FrequencyModulationSetting::FrequencyModulationSetting(const ArgType &arg)
    : m_frequency(arg.frequency),
      m_description(arg.description) {
}

unsigned int FrequencyModulationSetting::GenerateDescriptionResponse(
    uint8_t index,
    uint8_t *data) const {
  description_s *output = reinterpret_cast<description_s*>(data);
  output->setting = index;
  output->frequency = ola::network::HostToNetwork(m_frequency);
  strings::CopyToFixedLengthBuffer(m_description, output->description,
                                   arraysize(output->description));
  return (sizeof(description_s) - MAX_RDM_STRING_LENGTH + m_description.size());
}
}  // namespace rdm
}  // namespace ola
