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
 * E133StatusHelper.h
 * Functions for dealing with E1.33 Status Codes.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_E133_E133STATUSHELPER_H_
#define INCLUDE_OLA_E133_E133STATUSHELPER_H_

#include <stdint.h>
#include <ola/e133/E133Enums.h>
#include <string>

namespace ola {
namespace e133 {

using std::string;
using ola::e133::E133StatusCode;
using ola::e133::E133ConnectStatusCode;

bool IntToStatusCode(uint16_t input, E133StatusCode *status_code);
string StatusCodeToString(E133StatusCode status_code);

bool IntToConnectStatusCode(uint16_t input,
                            E133ConnectStatusCode *connect_status_code);
string ConnectStatusCodeToString(E133ConnectStatusCode connect_status_code);
}  // namespace e133
}  // namespace ola
#endif  // INCLUDE_OLA_E133_E133STATUSHELPER_H_
