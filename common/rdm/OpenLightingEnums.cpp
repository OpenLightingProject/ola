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
 * OpenLightingEnums.cpp
 * Copyright (C) 2013 Peter Newman
 */

#include "ola/rdm/OpenLightingEnums.h"

namespace ola {
namespace rdm {

const char OLA_MANUFACTURER_LABEL[] = "Open Lighting Project";
const char OLA_MANUFACTURER_URL[] = "https://openlighting.org/";

const char OLA_MANUFACTURER_PID_JSON_CODE_VERSION[] = "{\"name\":"
    "\"CODE_VERSION\",\"manufacturer_id\":31344,\"pid\":32769,\"version\":1,"
    "\"get_request_subdevice_range\":[\"root\",\"subdevices\"],"
    "\"get_request\":[],\"get_response\":[{\"name\":\"code_version\","
    "\"type\":\"string\",""\"maxLength\":32,\"restrictToASCII\":true}]}";
}  // namespace rdm
}  // namespace ola
