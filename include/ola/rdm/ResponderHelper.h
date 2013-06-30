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
 * ResponderHelper.h
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_RESPONDERHELPER_H_
#define INCLUDE_OLA_RDM_RESPONDERHELPER_H_

#include <string>
#include "ola/rdm/RDMCommand.h"

namespace ola {
namespace rdm {

/**
 * Helper methods for building RDM responders. These don't check that the
 * request is for the specified pid, so be sure to get it right!
 */
class ResponderHelper {
  public:
    static RDMResponse *GetDeviceInfo(const RDMRequest *request,
                                      uint16_t device_model,
                                      rdm_product_category product_category,
                                      uint32_t software_version,
                                      uint16_t dmx_footprint,
                                      uint8_t current_personality,
                                      uint8_t personality_count,
                                      uint16_t dmx_start_address,
                                      uint16_t sub_device_count,
                                      uint8_t sensor_count);

    static RDMResponse *GetRealTimeClock(const RDMRequest *request);

    static RDMResponse *GetString(const RDMRequest *request,
                                  const std::string &value);
    static RDMResponse *GetBoolValue(const RDMRequest *request, bool value);
    static RDMResponse *SetBoolValue(const RDMRequest *request, bool *value);
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RESPONDERHELPER_H_
