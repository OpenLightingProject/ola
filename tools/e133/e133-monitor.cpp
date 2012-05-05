/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * e133-monitor.cpp
 * Copyright (C) 2011 Simon Newton
 *
 * This locates all E1.33 devices using SLP and then opens a TCP connection to
 * each.  If --targets is used it skips the SLP step.
 *
 * It then waits to receive E1.33 messages on the TCP connections.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <errno.h>
#include <getopt.h>
#include <sysexits.h>

#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/Stringutils.h>
#include <ola/io/SelectServer.h>
#include <ola/network/AdvancedTCPConnector.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/Socket.h>
#include <ola/network/TCPSocketFactory.h>
#include <ola/rdm/CommandPrinter.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/UID.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include HASH_MAP_H

#include "plugins/e131/e131/ACNPort.h"
#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/RDMInflator.h"
#include "plugins/e131/e131/E133Inflator.h"
#include "plugins/e131/e131/RootInflator.h"
#include "plugins/e131/e131/RootSender.h"
#include "plugins/e131/e131/TCPTransport.h"

#include "tools/e133/E133Endpoint.h"
#include "tools/e133/E133HealthCheckedConnection.h"
#include "tools/e133/SlpThread.h"
#include "tools/e133/SlpUrlParser.h"

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::network::IPV4Address;
using ola::network::TcpSocket;
using ola::rdm::PidStoreHelper;
using ola::rdm::RDMCommand;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::TimeInterval;
using ola::TimeStamp;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using ola::plugin::e131::OutgoingStreamTransport;

typedef struct {
  bool help;
  ola::log_level log_level;
  string target_addresses;
  string pid_file;
} options;


/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {"pid-file", required_argument, 0, 'p'},
      {"targets", required_argument, 0, 't'},
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "hl:p:t:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 'h':
        opts->help = true;
        break;
      case 'l':
        switch (atoi(optarg)) {
          case 0:
            // nothing is written at this level
            // so this turns logging off
            opts->log_level = ola::OLA_LOG_NONE;
            break;
          case 1:
            opts->log_level = ola::OLA_LOG_FATAL;
            break;
          case 2:
            opts->log_level = ola::OLA_LOG_WARN;
            break;
          case 3:
            opts->log_level = ola::OLA_LOG_INFO;
            break;
          case 4:
            opts->log_level = ola::OLA_LOG_DEBUG;
            break;
          default :
            break;
        }
        break;
      case 'p':
        opts->pid_file = optarg;
        break;
      case 't':
        opts->target_addresses = optarg;
        break;
      case '?':
        break;
      default:
       break;
    }
  }
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char *argv[]) {
  cout << "Usage: " << argv[0] << " [options]\n"
  "\n"
  "Monitor E1.33 Devices.\n"
  "\n"
  "  -h, --help                Display this help message and exit.\n"
  "  -t, --targets <ip>,<ip>   List of IPs to connect to, overrides SLP\n"
  "  -p, --pid-file            The file to read PID definitions from\n"
  "  -l, --log-level <level>   Set the logging level 0 .. 4.\n"
  << endl;
  exit(0);
}


/**
 * Tracks the TCP connection state to a remote node
 */
class NodeTCPState {
  public:
    NodeTCPState()
      : socket(NULL),
        health_checked_connection(NULL),
        in_transport(NULL),
        out_transport(NULL),
        connection_attempts(0),
        am_master(false) {
    }
    ~NodeTCPState() {
      delete socket;
    }

    // public for now
    TcpSocket *socket;
    E133HealthCheckedConnection *health_checked_connection;
    ola::plugin::e131::IncomingTCPTransport *in_transport;
    ola::plugin::e131::OutgoingStreamTransport *out_transport;
    unsigned int connection_attempts;
    bool am_master;
};


/**
 * A very simple E1.33 Controller that acts as a passive monitor.
 */
class SimpleE133Monitor {
  public:
    explicit SimpleE133Monitor(PidStoreHelper *pid_helper);
    ~SimpleE133Monitor();

    bool Init();
    void PopulateResponderList();
    void AddIP(const IPV4Address &ip_address);

    void Run() { m_ss.Run(); }

  private:
    ola::rdm::CommandPrinter m_command_printer;
    ola::io::SelectServer m_ss;
    SlpThread m_slp_thread;
    ola::network::TCPSocketFactory m_tcp_socket_factory;
    ola::network::AdvancedTCPConnector m_connector;
    ola::network::LinearBackoffPolicy m_backoff_policy;

    // hash_map of ips to TCP Connection State
    typedef HASH_NAMESPACE::HASH_MAP_CLASS<uint32_t, NodeTCPState*> IPMap;
    IPMap m_ip_map;

    // The Controller's CID
    ola::plugin::e131::CID m_cid;

    // senders
    ola::plugin::e131::RootSender m_root_sender;

    // inflators
    ola::plugin::e131::RootInflator m_root_inflator;
    ola::plugin::e131::E133Inflator m_e133_inflator;
    ola::plugin::e131::RDMInflator m_rdm_inflator;

    /*
     * TODO: be careful about passing pointer to the NodeTCPState in callbacks
     * when we start removing stale entries this is going to break!
     * Maybe this won't be a problem since we'll never delete the entry for a
     * a node we have a connection to. Think about this.
     */
    void DiscoveryCallback(bool status, const vector<string> &urls);
    void OnTCPConnect(TcpSocket *socket);
    void ReceiveTCPData(IPV4Address ip_address,
                        ola::plugin::e131::IncomingTCPTransport *transport);
    void SocketUnhealthy(IPV4Address address);
    void SocketClosed(IPV4Address address);
    void E133DataReceived(const ola::plugin::e131::TransportHeader &header);

    void EndpointRequest(
        const ola::plugin::e131::TransportHeader &transport_header,
        const ola::plugin::e131::E133Header &e133_header,
        const string &raw_request);

    static const ola::TimeInterval TCP_CONNECT_TIMEOUT;
    static const ola::TimeInterval INITIAL_TCP_RETRY_DELAY;
    static const ola::TimeInterval MAX_TCP_RETRY_DELAY;
    static const char SOURCE_NAME[];
};

// 5 second connect() timeout
const ola::TimeInterval SimpleE133Monitor::TCP_CONNECT_TIMEOUT(5, 0);
// retry TCP connects after 5 seconds
const ola::TimeInterval SimpleE133Monitor::INITIAL_TCP_RETRY_DELAY(5, 0);
// we grow the retry interval to a max of 60 seconds
const ola::TimeInterval SimpleE133Monitor::MAX_TCP_RETRY_DELAY(60, 0);
const char SimpleE133Monitor::SOURCE_NAME[] = "OLA Monitor";


/**
 * Setup a new Monitori
 */
SimpleE133Monitor::SimpleE133Monitor(
    PidStoreHelper *pid_helper)
    : m_command_printer(&cout, pid_helper),
      m_slp_thread(
        &m_ss,
        ola::NewCallback(this, &SimpleE133Monitor::DiscoveryCallback)),
      m_tcp_socket_factory(NewCallback(this, &SimpleE133Monitor::OnTCPConnect)),
      m_connector(&m_ss, &m_tcp_socket_factory, TCP_CONNECT_TIMEOUT),
      m_backoff_policy(INITIAL_TCP_RETRY_DELAY, MAX_TCP_RETRY_DELAY),
      m_cid(ola::plugin::e131::CID::Generate()),
      m_root_sender(m_cid),
      m_root_inflator(NewCallback(this, &SimpleE133Monitor::E133DataReceived)) {
  m_root_inflator.AddInflator(&m_e133_inflator);
  m_e133_inflator.AddInflator(&m_rdm_inflator);

  m_rdm_inflator.SetRDMHandler(
      ROOT_E133_ENDPOINT,
      NewCallback(this, &SimpleE133Monitor::EndpointRequest));
}


SimpleE133Monitor::~SimpleE133Monitor() {
  // close out all tcp sockets and free state
  IPMap::iterator iter = m_ip_map.begin();
  for ( ; iter != m_ip_map.end(); ++iter) {
    delete iter->second;
  }
  m_ip_map.clear();

  m_slp_thread.Join();
  m_slp_thread.Cleanup();
}


bool SimpleE133Monitor::Init() {
  if (!m_slp_thread.Init()) {
    OLA_WARN << "SlpThread Init() failed";
    return false;
  }

  m_slp_thread.Start();
  return true;
}


/**
 * Locate the responder
 */
void SimpleE133Monitor::PopulateResponderList() {
  m_slp_thread.Discover();
}


void SimpleE133Monitor::AddIP(const IPV4Address &ip_address) {
  IPMap::iterator iter = m_ip_map.find(ip_address.AsInt());
  if (iter != m_ip_map.end()) {
    // the IP already exists
    return;
  }

  OLA_INFO << "Opening TCP connection to " << ip_address << ":" <<
    ola::plugin::e131::E133_PORT;

  NodeTCPState *node_state = new NodeTCPState();
  node_state->connection_attempts++;
  m_ip_map[ip_address.AsInt()] = node_state;

  // start the non-blocking connect
  m_connector.AddEndpoint(
      ip_address,
      ola::plugin::e131::E133_PORT,
      &m_backoff_policy);
}


/**
 * Called when SLP completes discovery.
 */
void SimpleE133Monitor::DiscoveryCallback(bool ok,
                                          const vector<string> &urls) {
  if (ok) {
    vector<string>::const_iterator iter;
    UID uid(0, 0);
    IPV4Address ip;
    for (iter = urls.begin(); iter != urls.end(); ++iter) {
      OLA_INFO << "Located " << *iter;
      if (!ParseSlpUrl(*iter, &uid, &ip))
        continue;

      if (uid.IsBroadcast()) {
        OLA_WARN << "UID " << uid << "@" << ip << " is broadcast";
        continue;
      }
      AddIP(ip);
    }
  } else {
    OLA_INFO << "SLP discovery failed";
  }
}


/**
 * Called when a TCP socket is connected. Note that we're not the master at
 * this point. That only happens if we receive data on the connection.
 */
void SimpleE133Monitor::OnTCPConnect(TcpSocket *socket) {
  IPV4Address ip_address;
  uint16_t port;
  socket->GetPeer(&ip_address, &port);

  IPMap::iterator iter = m_ip_map.find(ip_address.AsInt());
  if (iter == m_ip_map.end()) {
    OLA_FATAL << "Unable to locate socket for " << ip_address;
    if (socket) {
      socket->Close();
      delete socket;
    }
    return;
  }

  NodeTCPState *node_state = iter->second;

  // setup the incoming transport, we don't need to setup the outgoing one
  // until we've got confirmation that we're the master
  node_state->socket = socket;
  node_state->in_transport = new ola::plugin::e131::IncomingTCPTransport(
      &m_root_inflator,
      socket);

  socket->SetOnData(
      NewCallback(this,
                  &SimpleE133Monitor::ReceiveTCPData,
                  ip_address,
                  node_state->in_transport));
  socket->SetOnClose(
    NewSingleCallback(this, &SimpleE133Monitor::SocketClosed, ip_address));
  m_ss.AddReadDescriptor(socket);
}


/**
 * Receive data on a TCP connection
 */
void SimpleE133Monitor::ReceiveTCPData(
    IPV4Address ip_address,
    ola::plugin::e131::IncomingTCPTransport *transport) {
  bool ok = transport->Receive();
  if (!ok) {
    OLA_WARN << "TCP STREAM IS BAD!!!";
    SocketClosed(ip_address);
  }
}


/**
 * Called when a connection is deemed unhealthy.
 */
void SimpleE133Monitor::SocketUnhealthy(IPV4Address ip_address) {
  OLA_INFO << "connection to " << ip_address << " went unhealthy";
  SocketClosed(ip_address);
}


/**
 * Called when a socket is closed.
 * This can mean one of two things:
 *  if we weren't the master, then we lost the race.
 *  if we were the master the TCP connection was closed, or went unhealthy.
 */
void SimpleE133Monitor::SocketClosed(IPV4Address ip_address) {
  OLA_INFO << "connection to " << ip_address << " was closed";

  IPMap::iterator iter = m_ip_map.find(ip_address.AsInt());
  if (iter == m_ip_map.end()) {
    OLA_FATAL << "Unable to locate socket for " << ip_address;
    return;
  }

  NodeTCPState *node_state = iter->second;

  if (node_state->am_master) {
    // TODO(simon): signal other controllers here
    node_state->am_master = false;
    m_connector.Disconnect(ip_address, ola::plugin::e131::E133_PORT);
  } else {
    // we lost the race, so don't try to reconnect
    m_connector.Disconnect(ip_address, ola::plugin::e131::E133_PORT, true);
  }

  delete node_state->health_checked_connection;
  node_state->health_checked_connection = NULL;

  delete node_state->out_transport;
  node_state->out_transport = NULL;

  delete node_state->in_transport;
  node_state->in_transport = NULL;

  TcpSocket *socket = node_state->socket;
  m_ss.RemoveReadDescriptor(socket);
  delete socket;
  node_state->socket = NULL;

  // Terminate for now
  m_ss.Terminate();
}


/**
 * Called when we receive E1.33 data. If this arrived over TCP we notify the
 * health checked connection.
 */
void SimpleE133Monitor::E133DataReceived(
    const ola::plugin::e131::TransportHeader &header) {
  if (header.Transport() != ola::plugin::e131::TransportHeader::TCP)
    return;
  IPV4Address src_ip = header.SourceIP();

  IPMap::iterator iter = m_ip_map.find(src_ip.AsInt());
  if (iter == m_ip_map.end()) {
    OLA_FATAL << "Received data but unable to lookup socket for " <<
      header.SourceIP();
    return;
  }

  NodeTCPState *node_state = iter->second;

  // if we're already the master, we just need to notify the HealthChecker
  if (node_state->am_master) {
    node_state->health_checked_connection->HeartbeatReceived();
    return;
  }

  // this is the first packet received on this connection, which is a sign
  // we're now the master. Setup the HealthChecker & outgoing transports.
  node_state->am_master = true;
  OLA_INFO << "Now the master controller for " << header.SourceIP();

  OutgoingStreamTransport *outgoing_transport = new OutgoingStreamTransport(
      node_state->socket);

  E133HealthCheckedConnection *health_checked_connection =
      new E133HealthCheckedConnection(
          outgoing_transport,
          &m_root_sender,
          NewSingleCallback(this,
                            &SimpleE133Monitor::SocketUnhealthy,
                            src_ip),
          &m_ss);

  if (!health_checked_connection->Setup()) {
    OLA_WARN << "Failed to setup heartbeat controller for " << src_ip;
    SocketClosed(src_ip);
    return;
  }

  if (node_state->health_checked_connection)
    OLA_WARN << "pre-existing health_checked_connection for " << src_ip;

  if (node_state->out_transport)
    OLA_WARN << "pre-existing out_transport for " << src_ip;

  node_state->health_checked_connection = health_checked_connection;
  node_state->out_transport = outgoing_transport;
}


/**
 * We received data to endpoint 0
 */
void SimpleE133Monitor::EndpointRequest(
    const ola::plugin::e131::TransportHeader &transport_header,
    const ola::plugin::e131::E133Header&,
    const string &raw_request) {
  unsigned int slot_count = raw_request.size();
  const uint8_t *rdm_data = reinterpret_cast<const uint8_t*>(
    raw_request.data());

  if (slot_count < 25) {
    OLA_WARN << "RDM data from " << transport_header.SourceIP() <<
    " was too small: " << slot_count;
    return;
  }

  cout << "From " << transport_header.SourceIP() << ":" << endl;
  // switch based on response type
  const uint8_t response_type = rdm_data[19];
  bool ok = false;

  if (response_type == RDMCommand::GET_COMMAND ||
      response_type == RDMCommand::SET_COMMAND) {
    auto_ptr<RDMRequest> request(
        RDMRequest::InflateFromData(rdm_data, slot_count));
    if (request.get()) {
      m_command_printer.DisplayRequest(request.get());
      ok = true;
    }
  } else if (response_type == RDMCommand::GET_COMMAND_RESPONSE ||
             response_type == RDMCommand::SET_COMMAND_RESPONSE) {
    ola::rdm::rdm_response_code code;
    auto_ptr<RDMResponse> response(
        RDMResponse::InflateFromData(rdm_data, slot_count, &code));
    if (response.get()) {
      m_command_printer.DisplayResponse(response.get());
      ok = true;
    }
  }

  if (!ok)
    ola::FormatData(&cout, rdm_data, slot_count, 2);
}


/*
 * Startup a node
 */
int main(int argc, char *argv[]) {
  options opts;
  opts.pid_file = PID_DATA_FILE;
  opts.log_level = ola::OLA_LOG_WARN;
  opts.help = false;
  ParseOptions(argc, argv, &opts);
  PidStoreHelper pid_helper(opts.pid_file, 4);

  if (opts.help)
    DisplayHelpAndExit(argv);

  ola::InitLogging(opts.log_level, ola::OLA_LOG_STDERR);

  vector<IPV4Address> targets;
  if (!opts.target_addresses.empty()) {
    vector<string> tokens;
    ola::StringSplit(opts.target_addresses, tokens, ",");

    vector<string>::const_iterator iter = tokens.begin();
    for (; iter != tokens.end(); ++iter) {
      IPV4Address ip_address;
      if (!IPV4Address::FromString(*iter, &ip_address)) {
        OLA_WARN << "Invalid address " << *iter;
        DisplayHelpAndExit(argv);
      }
      targets.push_back(ip_address);
    }
  }

  if (!pid_helper.Init())
    exit(EX_OSFILE);

  SimpleE133Monitor monitor(&pid_helper);
  if (!monitor.Init())
    exit(EX_UNAVAILABLE);

  if (targets.empty()) {
    monitor.PopulateResponderList();
  } else {
    // manually add the responder IPs
    vector<IPV4Address>::const_iterator iter = targets.begin();
    for (; iter != targets.end(); ++iter)
      monitor.AddIP(*iter);
  }

  monitor.Run();
}
