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
 * MessageBuilder.cpp
 * Copyright (C) 2013 Simon Newton
 *
 * A class to simplify some of the E1.33 packet building operations.
 */

#include <string>
#include "ola/acn/CID.h"
#include "ola/e133/MessageBuilder.h"
#include "ola/io/IOStack.h"

#include "plugins/e131/e131/RootPDU.h"
#include "plugins/e131/e131/E133PDU.h"
#include "plugins/e131/e131/E133StatusPDU.h"
#include "plugins/e131/e131/PreamblePacker.h"
#include "plugins/e131/e131/ACNVectors.h"

namespace ola {
namespace e133 {

using ola::acn::CID;
using ola::io::IOStack;
using ola::plugin::e131::E133PDU;
using ola::plugin::e131::PreamblePacker;
using ola::plugin::e131::RootPDU;


MessageBuilder::MessageBuilder(const CID &cid, const string &source_name)
    : m_cid(cid),
      m_source_name(source_name),
      // The Max sized RDM packet is 256 bytes, E1.33 adds 118 bytes of
      // headers.
      m_memory_pool(400) {
}


/**
 * Build a NULL TCP packet. These packets can be used for heartbeats.
 */
void MessageBuilder::BuildNullTCPPacket(IOStack *packet) {
  RootPDU::PrependPDU(packet, ola::plugin::e131::VECTOR_ROOT_NULL, m_cid);
  PreamblePacker::AddTCPPreamble(packet);
}


/**
 * Build a TCP E1.33 Status PDU response. This should really only be used with
 * SC_E133_ACK.
 */
void MessageBuilder::BuildTCPE133StatusPDU(ola::io::IOStack *packet,
                                           uint32_t sequence_number,
                                           uint16_t endpoint_id,
                                           E133StatusCode status_code,
                                           const string &description) {
  ola::plugin::e131::E133StatusPDU::PrependPDU(
      packet, status_code, description);
  BuildTCPRootE133(
      packet, ola::plugin::e131::VECTOR_FRAMING_STATUS,
      sequence_number, endpoint_id);
}


/**
 * Build an E1.33 Status PDU response
 */
void MessageBuilder::BuildUDPE133StatusPDU(ola::io::IOStack *packet,
                                           uint32_t sequence_number,
                                           uint16_t endpoint_id,
                                           E133StatusCode status_code,
                                           const string &description) {
  ola::plugin::e131::E133StatusPDU::PrependPDU(
      packet, status_code, description);
  BuildUDPRootE133(
      packet, ola::plugin::e131::VECTOR_FRAMING_STATUS,
      sequence_number, endpoint_id);
}


/**
 * Append an E133PDU, a RootPDU and the TCP preamble to a packet.
 */
void MessageBuilder::BuildTCPRootE133(IOStack *packet,
                                      uint32_t vector,
                                      uint32_t sequence_number,
                                      uint16_t endpoint_id) {
  E133PDU::PrependPDU(packet, vector, m_source_name, sequence_number,
                      endpoint_id);
  RootPDU::PrependPDU(packet, ola::plugin::e131::VECTOR_ROOT_E133, m_cid);
  PreamblePacker::AddTCPPreamble(packet);
}


/**
 * Append an E133PDU, a RootPDU and the UDP preamble to a packet.
 */
void MessageBuilder::BuildUDPRootE133(IOStack *packet,
                                      uint32_t vector,
                                      uint32_t sequence_number,
                                      uint16_t endpoint_id) {
  E133PDU::PrependPDU(packet, vector, m_source_name, sequence_number,
                      endpoint_id);
  RootPDU::PrependPDU(packet, ola::plugin::e131::VECTOR_ROOT_E133, m_cid);
  PreamblePacker::AddUDPPreamble(packet);
}
}  // e133
}  // ola
