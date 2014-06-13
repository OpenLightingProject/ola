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
 * SLPUtil.h
 * Utility functions for SLP
 * Copyright (C) 2012 Simon Newton
 */

#ifndef SLP_SLPUTIL_H_
#define SLP_SLPUTIL_H_

#include <string>
#include <set>
#include <algorithm>
#include "slp/SLPPacketConstants.h"

namespace ola {
namespace slp {

using std::string;

string SLPErrorToString(uint16_t error);
}  // namespace slp
}  // namespace ola
#endif  // SLP_SLPUTIL_H_
