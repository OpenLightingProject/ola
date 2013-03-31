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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * E133StatusHelper.h
 * Functions for dealing with E1.33 Status Codes.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_E131_E131_E133STATUSHELPER_H_
#define PLUGINS_E131_E131_E133STATUSHELPER_H_

#include <stdint.h>
#include <string>
#include "plugins/e131/e131/E133Enums.h"

namespace ola {
namespace plugin {
namespace e131 {

using std::string;

bool IntToStatusCode(uint16_t input, E133StatusCode *status_code);
string StatusMessageIdToString(E133StatusCode status_code);
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_E133STATUSHELPER_H_
