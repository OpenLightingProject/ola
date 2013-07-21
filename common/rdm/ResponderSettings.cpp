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
 * ResponderSettings.cpp
 * Copyright (C) 2013 Simon Newton
 */


#include <ola/network/NetworkUtils.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/ResponderSettings.h>
#include <string>

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
  strncpy(output->description, m_description.c_str(), MAX_RDM_STRING_LENGTH);
  return (sizeof(description_s) - MAX_RDM_STRING_LENGTH +
          strlen(output->description));
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
  strncpy(output->description, m_description.c_str(), MAX_RDM_STRING_LENGTH);
  return (sizeof(description_s) - MAX_RDM_STRING_LENGTH +
          strlen(output->description));
}
}  // namespace rdm
}  // namespace ola
