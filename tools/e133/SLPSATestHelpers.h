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
 * SLPSATestHelpers.h
 * Copyright (C) 2013 Simon Newton
 */

#ifndef TOOLS_E133_SLPSATESTHELPERS_H_
#define TOOLS_E133_SLPSATESTHELPERS_H_

#include <ola/io/BigEndianStream.h>
#include <ola/network/IPV4Address.h>
#include <string>

#include "tools/e133/SLPSATestRunner.h"
#include "slp/ScopeSet.h"

using ola::io::BigEndianOutputStream;
using ola::network::IPV4Address;
using ola::slp::ScopeSet;

extern const char RDMNET_DEVICE_SERVICE[] = "service:rdmnet-device";
extern const ScopeSet RDMNET_SCOPES("rdmnet");

void BuildNLengthPacket(BigEndianOutputStream *output, uint8_t data,
                        unsigned int length);
void WriteOverflowString(BigEndianOutputStream *output,
                         unsigned int header_size,
                         unsigned int actual_size);
TestCase::TestState VerifyEmptySrvReply(const uint8_t *data,
                                        unsigned int length);
TestCase::TestState VerifySrvRply(const IPV4Address &destination_ip,
                                  const uint8_t *data, unsigned int length);

void BuildPRListOverflowSrvRqst(BigEndianOutputStream* output, bool multicast,
                                xid_t xid);
void BuildServiceTypeOverflowSrvRqst(BigEndianOutputStream* output,
                                     bool multicast, xid_t xid);
void BuildScopeListOverflowSrvRqst(BigEndianOutputStream* output,
                                   bool multicast, xid_t xid);
void BuildPredicateOverflowSrvRqst(BigEndianOutputStream* output,
                                   bool multicast, xid_t xid);
void BuildSPIOverflowSrvRqst(BigEndianOutputStream* output,
                             bool multicast, xid_t xid);

#endif  // TOOLS_E133_SLPSATESTHELPERS_H_
