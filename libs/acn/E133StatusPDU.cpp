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
 * E133StatusPDU.cpp
 * The E133StatusPDU
 * Copyright (C) 2013 Simon Newton
 */

#include <ola/network/NetworkUtils.h>
#include <algorithm>
#include <string>
#include "libs/acn/E133StatusPDU.h"

namespace ola {
namespace acn {

using ola::network::HostToNetwork;
using std::string;

void E133StatusPDU::PrependPDU(ola::io::IOStack *stack,
                               ola::e133::E133StatusCode status_code_enum,
                               const string &status) {
  const string truncated_status_code = status.substr(
      0,
      std::min(status.size(),
               static_cast<size_t>(ola::e133::MAX_E133_STATUS_STRING_SIZE)));
  stack->Write(reinterpret_cast<const uint8_t*>(truncated_status_code.data()),
               static_cast<unsigned int>(truncated_status_code.size()));
  uint16_t status_code = HostToNetwork(static_cast<uint16_t>(status_code_enum));
  stack->Write(reinterpret_cast<uint8_t*>(&status_code),
               sizeof(status_code));
  PrependFlagsAndLength(stack);
}
}  // namespace acn
}  // namespace ola
