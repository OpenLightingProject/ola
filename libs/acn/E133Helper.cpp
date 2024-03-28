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
 * E133Helper.cpp
 * Various misc E1.33 functions.
 * Copyright (C) 2024 Peter Newman
 *
 * At some point we may want to localize this file.
 */

#include <sstream>
#include <string>
#include <vector>
#include "ola/e133/E133Enums.h"
#include "ola/e133/E133Helper.h"
#include "ola/StringUtils.h"

namespace ola {
namespace e133 {

using std::ostringstream;
using std::string;
using std::vector;


/**
 * Verify that the int is a valid E1.33 RPT Client Type.
 */
bool IntToRPTClientType(uint8_t input,
                        ola::e133::E133RPTClientTypeCode *client_type) {
  switch (input) {
    case ola::e133::RPT_CLIENT_TYPE_DEVICE:
      *client_type = ola::e133::RPT_CLIENT_TYPE_DEVICE;
      return true;
    case ola::e133::RPT_CLIENT_TYPE_CONTROLLER:
      *client_type = ola::e133::RPT_CLIENT_TYPE_CONTROLLER;
      return true;
    default:
      return false;
  }
}


/**
 * Convert a uint8_t representing an RPT client type to a human-readable string.
 * @param type the RPT client type value
 */
string RPTClientTypeToString(uint8_t type) {
  switch (type) {
    case RPT_CLIENT_TYPE_DEVICE:
      return "Device";
    case RPT_CLIENT_TYPE_CONTROLLER:
      return "Controller";
    default:
      ostringstream str;
      str << "Unknown, was " << static_cast<int>(type);
      return str.str();
  }
}
}  // namespace e133
}  // namespace  ola
