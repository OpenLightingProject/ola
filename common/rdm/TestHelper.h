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
 * TestHelper.h
 * Helper classes for the RDM tests.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef COMMON_RDM_TESTHELPER_H_
#define COMMON_RDM_TESTHELPER_H_

#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMCommand.h"

inline uint16_t NackReasonFromResponse(const ola::rdm::RDMResponse *response) {
  uint16_t reason;
  memcpy(reinterpret_cast<uint8_t*>(&reason), response->ParamData(),
         sizeof(reason));
  return ola::network::NetworkToHost(reason);
}

#endif  // COMMON_RDM_TESTHELPER_H_
