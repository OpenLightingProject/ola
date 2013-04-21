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

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>

#include <ola/BaseTypes.h>
#include <ola/Logging.h>
#include <ola/acn/ACNPort.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/e133/SLPThread.h>
#include <ola/io/SelectServer.h>
#include <ola/io/StdinHandler.h>
#include <ola/network/InterfacePicker.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/UID.h>

#include <iostream>
#include <memory>
#include <string>

#include "plugins/dummy/DummyResponder.h"

#include "tools/e133/E133Device.h"
#include "tools/e133/EndpointManager.h"
#include "tools/e133/ManagementEndpoint.h"
#include "tools/e133/TCPConnectionStats.h"

using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;

DEFINE_string(listen_ip, "", "The IP address to listen on.");
DEFINE_s_uint16(lifetime, t, 300, "The value to use for the service lifetime");
DEFINE_string(uid, "7a70:00000001", "The UID of the responder.");
DEFINE_s_uint32(universe, u, 1, "The E1.31 universe to listen on.");

/**
 * A very simple E1.33 node that registers itself using SLP and responds to
 * messages.
 */
class SimpleE133Node {
  public:
    struct Options {
      IPV4Address ip_address;
      UID uid;
      uint16_t lifetime;

      Options(const IPV4Address &ip, const UID &uid, uint16_t lifetime)
        : ip_address(ip), uid(uid), lifetime(lifetime) {
      }
    };

    explicit SimpleE133Node(const Options &options);
    ~SimpleE133Node();

    bool Init();
    void Run();
    void Stop() { m_ss.Terminate(); }

  private:
    ola::io::SelectServer m_ss;
    auto_ptr<ola::e133::BaseSLPThread> m_slp_thread;
    ola::io::StdinHandler m_stdin_handler;
    EndpointManager m_endpoint_manager;
    E133Device m_e133_device;
    ManagementEndpoint m_management_endpoint;
    E133Endpoint m_first_endpoint;
    ola::plugin::dummy::DummyResponder m_responder;
    const uint16_t m_lifetime;
    const UID m_uid;
    const IPV4Address m_ip_address;

    void RegisterCallback(bool ok);
    void DeRegisterCallback(bool ok);

    void Input(char c);
    void DumpTCPStats();
    void SendUnsolicited();

    SimpleE133Node(const SimpleE133Node&);
    SimpleE133Node operator=(const SimpleE133Node&);
};


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
      m_first_endpoint(NULL, E133Endpoint::EndpointProperties()),
      m_responder(options.uid),
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
  // add a single endpoint
  m_endpoint_manager.RegisterEndpoint(1, &m_first_endpoint);

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


SimpleE133Node *simple_node;

/*
 * Terminate cleanly on interrupt.
 */
static void InteruptSignal(int signo) {
  if (simple_node)
    simple_node->Stop();
  (void) signo;
}


/*
 * Startup a node
 */
int main(int argc, char *argv[]) {
  ola::SetHelpString(
      "[options]",
      "Run a very simple E1.33 Responder.");
  ola::ParseFlags(&argc, argv);
  ola::InitLoggingFromFlags();

  auto_ptr<UID> uid(UID::FromString(FLAGS_uid));
  if (!uid.get()) {
    OLA_WARN << "Invalid UID: " << FLAGS_uid;
    ola::DisplayUsage();
    exit(EX_USAGE);
  }

  ola::network::Interface interface;

  {
    auto_ptr<const ola::network::InterfacePicker> picker(
      ola::network::InterfacePicker::NewPicker());
    if (!picker->ChooseInterface(&interface, FLAGS_listen_ip)) {
      OLA_INFO << "Failed to find an interface";
      exit(EX_UNAVAILABLE);
    }
  }

  SimpleE133Node::Options opts(interface.ip_address, *uid, FLAGS_lifetime);
  SimpleE133Node node(opts);
  simple_node = &node;

  if (!node.Init())
    exit(EX_UNAVAILABLE);

  // signal handler
  if (!ola::InstallSignal(SIGINT, &InteruptSignal))
    return false;

  node.Run();
}
