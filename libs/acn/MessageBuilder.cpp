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
 * MessageBuilder.cpp
 * Copyright (C) 2013 Simon Newton
 *
 * A class to simplify some of the E1.33 packet building operations.
 */

#include <string>
#include "ola/acn/ACNVectors.h"
#include "ola/acn/CID.h"
#include "ola/e133/MessageBuilder.h"
#include "ola/io/IOStack.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/UID.h"

#include "libs/acn/BrokerPDU.h"
#include "libs/acn/E133PDU.h"
#include "libs/acn/RDMPDU.h"
#include "libs/acn/RootPDU.h"
#include "libs/acn/RPTPDU.h"
#include "libs/acn/RPTRequestPDU.h"
#include "libs/acn/E133StatusPDU.h"
#include "libs/acn/PreamblePacker.h"

namespace ola {
namespace e133 {

using ola::acn::CID;
using ola::io::IOStack;
using ola::acn::BrokerPDU;
using ola::acn::E133PDU;
using ola::acn::PreamblePacker;
using ola::acn::RootPDU;
using ola::acn::RPTPDU;


MessageBuilder::MessageBuilder(const CID &cid, const string &source_name)
    : m_cid(cid),
      m_source_name(source_name),
      // The Max sized RDM packet is 256 bytes, E1.33 adds 118 bytes of
      // headers.
      m_memory_pool(400) {
}


/**
 * Append a RDM PDU Header onto this packet
 */
void MessageBuilder::PrependRDMHeader(IOStack *packet) {
  ola::acn::RDMPDU::PrependPDU(packet);
}


/**
 * Build a TCP E1.33 RDM Command PDU response.
 */
void MessageBuilder::BuildTCPRDMCommandPDU(IOStack *packet,
                                           ola::rdm::RDMRequest *request,
                                           uint16_t source_endpoint_id,
                                           uint16_t destination_endpoint_id,
                                           uint32_t sequence_number) {
  // TODO(Peter): Potentially need some future way to handle controller
  // messages here
  ola::rdm::UID rpt_destination_uid = request->DestinationUID();
  if (rpt_destination_uid.IsBroadcast()) {
    if (rpt_destination_uid.IsVendorcast()) {
      rpt_destination_uid = ola::rdm::UID::RPTVendorcastAddressDevices(
          rpt_destination_uid);
    } else {
      rpt_destination_uid = ola::rdm::UID::RPTAllDevices();
    }
    if (destination_endpoint_id != NULL_ENDPOINT) {
      // TODO(Peter): Should we handle the reserved endpoints now?
      destination_endpoint_id = BROADCAST_ENDPOINT;
    }
  }
  ola::rdm::RDMCommandSerializer::Write(*request, packet);
  ola::acn::RDMPDU::PrependPDU(packet);
  ola::acn::RPTRequestPDU::PrependPDU(packet);
  RPTPDU::PrependPDU(packet, ola::acn::VECTOR_RPT_REQUEST,
                     request->SourceUID(), source_endpoint_id,
                     rpt_destination_uid, destination_endpoint_id,
                     sequence_number);
  RootPDU::PrependPDU(packet, ola::acn::VECTOR_ROOT_RPT, m_cid, true);
  PreamblePacker::AddTCPPreamble(packet);
}


/**
 * Build a NULL TCP packet. These packets can be used for heartbeats.
 */
void MessageBuilder::BuildNullTCPPacket(IOStack *packet) {
  RootPDU::PrependPDU(packet, ola::acn::VECTOR_ROOT_NULL, m_cid);
  PreamblePacker::AddTCPPreamble(packet);
}


/**
 * Build a Broker Fetch Client List TCP packet.
 */
void MessageBuilder::BuildBrokerFetchClientListTCPPacket(IOStack *packet) {
  BrokerPDU::PrependPDU(packet, ola::acn::VECTOR_BROKER_FETCH_CLIENT_LIST);
  RootPDU::PrependPDU(packet, ola::acn::VECTOR_ROOT_BROKER, m_cid, true);
  PreamblePacker::AddTCPPreamble(packet);
}


/**
 * Build a Broker NULL TCP packet. These packets can be used for broker heartbeats.
 */
void MessageBuilder::BuildBrokerNullTCPPacket(IOStack *packet) {
  BrokerPDU::PrependPDU(packet, ola::acn::VECTOR_BROKER_NULL);
  RootPDU::PrependPDU(packet, ola::acn::VECTOR_ROOT_BROKER, m_cid, true);
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
  ola::acn::E133StatusPDU::PrependPDU(
      packet, status_code, description);
  BuildTCPRootE133(
      packet, ola::acn::VECTOR_FRAMING_STATUS,
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
  ola::acn::E133StatusPDU::PrependPDU(
      packet, status_code, description);
  BuildUDPRootE133(
      packet, ola::acn::VECTOR_FRAMING_STATUS,
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
  RootPDU::PrependPDU(packet, ola::acn::VECTOR_ROOT_RPT, m_cid);
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
  RootPDU::PrependPDU(packet, ola::acn::VECTOR_ROOT_RPT, m_cid);
  PreamblePacker::AddUDPPreamble(packet);
}
}  // namespace e133
}  // namespace ola
