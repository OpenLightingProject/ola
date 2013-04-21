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
 * e133-receiver.cpp
 * Copyright (C) 2011 Simon Newton
 *
 * This creates a E1.33 receiver with one (emulated) RDM responder. The node is
 * registered in slp and the RDM responder responds to E1.33 commands.
 */

#include <ola/BaseTypes.h>
#include <ola/Logging.h>
#include <ola/acn/ACNPort.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/e133/SLPThread.h>
#include <ola/io/SelectServer.h>
#include <ola/io/StdinHandler.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/UID.h>

#include <iostream>
#include <string>

#include "plugins/dummy/DummyResponder.h"
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
    : m_slp_thread(ola::e133::SLPThreadFactory::NewSLPThread(&m_ss)),
      m_stdin_handler(&m_ss, ola::NewCallback(this, &SimpleE133Node::Input)),
      m_e133_device(&m_ss, options.ip_address, &m_endpoint_manager),
      m_management_endpoint(NULL, E133Endpoint::EndpointProperties(),
                            options.uid, &m_endpoint_manager,
                            m_e133_device.GetTCPStats()),
      m_lifetime(options.lifetime),
      m_uid(options.uid),
      m_ip_address(options.ip_address) {
}


SimpleE133Node::~SimpleE133Node() {
  m_endpoint_manager.UnRegisterEndpoint(1);
  m_slp_thread->Join(NULL);
  m_slp_thread->Cleanup();
}


/**
 * Init this node
 */
bool SimpleE133Node::Init() {
  if (!m_e133_device.Init())
    return false;

  // register the root endpoint
  m_e133_device.SetRootEndpoint(&m_management_endpoint);

  // Start the SLP thread.
  if (!m_slp_thread->Init()) {
    OLA_WARN << "SLPThread Init() failed";
    return false;
  }
  m_slp_thread->Start();

  cout << "---------------  Controls  ----------------\n";
  cout << " c - Close the TCP connection\n";
  cout << " q - Quit\n";
  cout << " s - Send Status Message\n";
  cout << " t - Dump TCP stats\n";
  cout << "-------------------------------------------\n";
  return true;
}


void SimpleE133Node::Run() {
  m_slp_thread->RegisterDevice(
    ola::NewSingleCallback(this, &SimpleE133Node::RegisterCallback),
    m_ip_address, m_uid, m_lifetime);

  m_ss.Run();
  OLA_INFO << "Starting shutdown process";

  m_slp_thread->DeRegisterDevice(
    ola::NewSingleCallback(this, &SimpleE133Node::DeRegisterCallback),
    m_ip_address, m_uid);
  m_ss.Run();
}

void SimpleE133Node::AddEndpoint(uint16_t endpoint_id,
                                 E133Endpoint *endpoint) {
  m_endpoint_manager.RegisterEndpoint(endpoint_id, endpoint);
}

void SimpleE133Node::RemoveEndpoint(uint16_t endpoint_id) {
  m_endpoint_manager.UnRegisterEndpoint(endpoint_id);
}

/**
 * Called when a registration request completes.
 */
void SimpleE133Node::RegisterCallback(bool ok) {
  if (!ok)
    OLA_WARN << "Failed to register in SLP";
}


/**
 * Called when a de-registration request completes.
 */
void SimpleE133Node::DeRegisterCallback(bool ok) {
  if (!ok)
    OLA_WARN << "Failed to de-register in SLP";
  m_ss.Terminate();
}


/**
 * Called when there is data on stdin.
 */
void SimpleE133Node::Input(char c) {
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
 * Send an unsolicted message on the TCP connection
 */
void SimpleE133Node::SendUnsolicited() {
  OLA_INFO << "Sending unsolicited TCP stats message";

  struct tcp_stats_message_s {
    uint32_t ip_address;
    uint16_t unhealthy_events;
    uint16_t connection_events;
  } __attribute__((packed));

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
