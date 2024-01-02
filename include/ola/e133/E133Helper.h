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
 * E133Helper.h
 * Various misc E1.33 functions.
 * Copyright (C) 2024 Peter Newman
 */

#ifndef INCLUDE_OLA_E133_E133HELPER_H_
#define INCLUDE_OLA_E133_E133HELPER_H_

#include <stdint.h>
#include <ola/e133/E133Enums.h>
#include <string>

namespace ola {
namespace e133 {

bool IntToRPTClientType(uint8_t input,
                        ola::e133::E133RPTClientTypeCode *client_type);
std::string RPTClientTypeToString(uint8_t type);
}  // namespace e133
}  // namespace ola
#endif  // INCLUDE_OLA_E133_E133HELPER_H_
