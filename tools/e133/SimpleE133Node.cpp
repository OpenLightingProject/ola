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
 * SimpleE133Node.cpp
 * Copyright (C) 2011 Simon Newton
 *
 * This creates a E1.33 receiver with one (emulated) RDM responder. The node's
 * RDM responder responds to E1.33 commands.
 */

#include <ola/Constants.h>
#include <ola/Logging.h>
#include <ola/acn/ACNPort.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/io/SelectServer.h>
#include <ola/io/StdinHandler.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/UID.h>

#include <iostream>
#include <string>

#include "tools/e133/E133Device.h"
#include "tools/e133/EndpointManager.h"
#include "tools/e133/ManagementEndpoint.h"
#include "tools/e133/SimpleE133Node.h"
#include "tools/e133/TCPConnectionStats.h"

using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using std::cout;
using std::endl;
using std::string;

/**
 * Constructor
 */
SimpleE133Node::SimpleE133Node(const Options &options)
    : m_stdin_handler(&m_ss, ola::NewCallback(this, &SimpleE133Node::Input)),
      m_e133_device(&m_ss, options.cid, options.ip_address,
                    &m_endpoint_manager),
      m_management_endpoint(NULL, E133Endpoint::EndpointProperties(),
                            options.uid, &m_endpoint_manager,
                            m_e133_device.GetTCPStats()),
      m_lifetime(options.lifetime),
      m_uid(options.uid),
      m_ip_address(options.ip_address) {
}


SimpleE133Node::~SimpleE133Node() {
  m_endpoint_manager.UnRegisterEndpoint(1);
}


/**
 * Init this node
 */
bool SimpleE133Node::Init() {
  if (!m_e133_device.Init())
    return false;

  // register the root endpoint
  m_e133_device.SetRootEndpoint(&m_management_endpoint);

  cout << "---------------  Controls  ----------------\n";
  cout << " c - Close the TCP connection\n";
  cout << " q - Quit\n";
  cout << " s - Send Status Message\n";
  cout << " t - Dump TCP stats\n";
  cout << "-------------------------------------------\n";
  return true;
}


void SimpleE133Node::Run() {
  m_ss.Run();
  OLA_INFO << "Starting shutdown process";
}

void SimpleE133Node::AddEndpoint(uint16_t endpoint_id,
                                 E133Endpoint *endpoint) {
  m_endpoint_manager.RegisterEndpoint(endpoint_id, endpoint);
}

void SimpleE133Node::RemoveEndpoint(uint16_t endpoint_id) {
  m_endpoint_manager.UnRegisterEndpoint(endpoint_id);
}

/**
 * Called when there is data on stdin.
 */
void SimpleE133Node::Input(int c) {
  switch (c) {
    case 'c':
      m_e133_device.CloseTCPConnection();
      break;
    case 'q':
      m_ss.Terminate();
      break;
    case 's':
      SendUnsolicited();
      break;
    case 't':
      DumpTCPStats();
      break;
    default:
      break;
  }
}


/**
 * Dump the TCP stats
 */
void SimpleE133Node::DumpTCPStats() {
  const TCPConnectionStats* stats = m_e133_device.GetTCPStats();
  cout << "IP: " << stats->ip_address << endl;
  cout << "Connection Unhealthy Events: " << stats->unhealthy_events <<
    endl;
  cout << "Connection Events: " << stats->connection_events << endl;
}


/**
 * Send an unsolicited message on the TCP connection
 */
void SimpleE133Node::SendUnsolicited() {
  OLA_INFO << "Sending unsolicited TCP stats message";

  PACK(
  struct tcp_stats_message_s {
    uint32_t ip_address;
    uint16_t unhealthy_events;
    uint16_t connection_events;
  });

  struct tcp_stats_message_s tcp_stats_message;

  const TCPConnectionStats* stats = m_e133_device.GetTCPStats();
  tcp_stats_message.ip_address = stats->ip_address.AsInt();
  tcp_stats_message.unhealthy_events =
    HostToNetwork(stats->unhealthy_events);
  tcp_stats_message.connection_events =
    HostToNetwork(stats->connection_events);

  UID bcast_uid = UID::AllDevices();
  const RDMResponse *response = new ola::rdm::RDMGetResponse(
      m_uid,
      bcast_uid,
      0,  // transaction number
      ola::rdm::RDM_ACK,
      0,  // message count
      ola::rdm::ROOT_RDM_DEVICE,
      ola::rdm::PID_TCP_COMMS_STATUS,
      reinterpret_cast<const uint8_t*>(&tcp_stats_message),
      sizeof(tcp_stats_message));

  m_e133_device.SendStatusMessage(response);
}
