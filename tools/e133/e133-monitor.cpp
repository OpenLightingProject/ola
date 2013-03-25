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
#include <ola/stl/STLUtils.h>

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
#include "tools/e133/OLASLPThread.h"
#ifdef HAVE_LIBSLP
#include "tools/e133/OpenSLPThread.h"
#endif
#include "tools/e133/SLPThread.h"
#include "tools/e133/E133URLParser.h"
#include "tools/slp/URLEntry.h"

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::STLContains;
using ola::TimeInterval;
using ola::TimeStamp;
using ola::network::BufferedTCPSocket;
using ola::network::GenericSocketAddress;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::plugin::e131::OutgoingStreamTransport;
using ola::rdm::PidStoreHelper;
using ola::rdm::RDMCommand;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::slp::URLEntries;

using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

typedef struct {
  bool help;
  bool use_openslp;
  ola::log_level log_level;
  string target_addresses;
  string pid_location;
} options;


/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  enum {
    OPENSLP_OPTION = 256,
  };

  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {"pid-location", required_argument, 0, 'p'},
      {"targets", required_argument, 0, 't'},
#ifdef HAVE_LIBSLP
      {"openslp", no_argument, 0, OPENSLP_OPTION},
#endif
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
        opts->pid_location = optarg;
        break;
      case 't':
        opts->target_addresses = optarg;
        break;
      case '?':
        break;
      case OPENSLP_OPTION:
        opts->use_openslp = true;
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
  "  -p, --pid-location        The directory to read PID definitiions from\n"
  "  -l, --log-level <level>   Set the logging level 0 .. 4.\n"
#ifdef HAVE_LIBSLP
  "  --openslp                 Use openslp rather than the OLA SLP server\n"
#endif
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

    // public for now
    auto_ptr<BufferedTCPSocket> socket;
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
    enum SLPOption {
      OPEN_SLP,
      OLA_SLP,
      NO_SLP,
    };
    explicit SimpleE133Monitor(PidStoreHelper *pid_helper,
                               SLPOption slp_option);
    ~SimpleE133Monitor();

    bool Init();
    void AddIP(const IPV4Address &ip_address);

    void Run() { m_ss.Run(); }

  private:
    ola::rdm::CommandPrinter m_command_printer;
    ola::io::SelectServer m_ss;
    auto_ptr<BaseSLPThread> m_slp_thread;
    ola::network::BufferedTCPSocketFactory m_tcp_socket_factory;
    ola::network::AdvancedTCPConnector m_connector;
    ola::LinearBackoffPolicy m_backoff_policy;

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
    void DiscoveryCallback(bool status, const URLEntries &urls);
    void OnTCPConnect(BufferedTCPSocket *socket);
    void ReceiveTCPData(IPV4Address ip_address,
                        ola::plugin::e131::IncomingTCPTransport *transport);
    void SocketUnhealthy(IPV4Address address);
    void SocketClosed(IPV4Address address);
    void RLPDataReceived(const ola::plugin::e131::TransportHeader &header);

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
// we grow the retry interval to a max of 30 seconds
const ola::TimeInterval SimpleE133Monitor::MAX_TCP_RETRY_DELAY(30, 0);
const char SimpleE133Monitor::SOURCE_NAME[] = "OLA Monitor";


/**
 * Setup a new Monitor
 */
SimpleE133Monitor::SimpleE133Monitor(
    PidStoreHelper *pid_helper,
    SLPOption slp_option)
    : m_command_printer(&cout, pid_helper),
      m_tcp_socket_factory(NewCallback(this, &SimpleE133Monitor::OnTCPConnect)),
      m_connector(&m_ss, &m_tcp_socket_factory, TCP_CONNECT_TIMEOUT),
      m_backoff_policy(INITIAL_TCP_RETRY_DELAY, MAX_TCP_RETRY_DELAY),
      m_cid(ola::plugin::e131::CID::Generate()),
      m_root_sender(m_cid),
      m_root_inflator(NewCallback(this, &SimpleE133Monitor::RLPDataReceived)) {
  if (slp_option == OLA_SLP) {
    m_slp_thread.reset(new OLASLPThread(&m_ss));
  } else if (slp_option == OPEN_SLP) {
#ifdef HAVE_LIBSLP
    m_slp_thread.reset(new OpenSLPThread(&m_ss));
#else
    OLA_WARN << "openslp not installed";
#endif
  }
  if (m_slp_thread.get()) {
    m_slp_thread->SetNewDeviceCallback(
      NewCallback(this, &SimpleE133Monitor::DiscoveryCallback));
  }
  // TODO(simon): add a controller discovery callback here as well.

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

  if (m_slp_thread.get()) {
    m_slp_thread->Join(NULL);
    m_slp_thread->Cleanup();
  }
}


bool SimpleE133Monitor::Init() {
  if (!m_slp_thread.get())
    return true;

  if (!m_slp_thread->Init()) {
    OLA_WARN << "SLPThread Init() failed";
    return false;
  }

  m_slp_thread->Start();
  return true;
}


void SimpleE133Monitor::AddIP(const IPV4Address &ip_address) {
  if (STLContains(m_ip_map, ip_address.AsInt())) {
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
      IPV4SocketAddress(ip_address, ola::plugin::e131::E133_PORT),
      &m_backoff_policy);
}


/**
 * Called when SLP completes discovery.
 */
void SimpleE133Monitor::DiscoveryCallback(bool ok, const URLEntries &urls) {
  if (ok) {
    URLEntries::const_iterator iter;
    UID uid(0, 0);
    IPV4Address ip;
    for (iter = urls.begin(); iter != urls.end(); ++iter) {
      OLA_INFO << "Located " << *iter;
      if (!ParseE133URL(iter->url(), &uid, &ip))
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
void SimpleE133Monitor::OnTCPConnect(BufferedTCPSocket *socket) {
  GenericSocketAddress address = socket->GetPeer();
  if (address.Family() != AF_INET) {
    OLA_WARN << "Non IPv4 socket " << address;
    delete socket;
    return;
  }
  IPV4SocketAddress v4_address = address.V4Addr();
  NodeTCPState *node_state = ola::STLFindOrNull(
      m_ip_map, v4_address.Host().AsInt());
  if (!node_state) {
    OLA_FATAL << "Unable to locate socket for " << v4_address.Host();
    delete socket;
  }

  // setup the incoming transport, we don't need to setup the outgoing one
  // until we've got confirmation that we're the master
  node_state->socket.reset(socket);
  node_state->in_transport = new ola::plugin::e131::IncomingTCPTransport(
      &m_root_inflator,
      socket);

  socket->SetOnData(
      NewCallback(this,
                  &SimpleE133Monitor::ReceiveTCPData,
                  v4_address.Host(),
                  node_state->in_transport));
  socket->SetOnClose(
    NewSingleCallback(this, &SimpleE133Monitor::SocketClosed,
                      v4_address.Host()));
  m_ss.AddReadDescriptor(socket);

  // setup a timeout that closes this connect if we don't receive anything
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

  NodeTCPState *node_state = ola::STLFindOrNull(m_ip_map, ip_address.AsInt());
  if (!node_state) {
    OLA_FATAL << "Unable to locate socket for " << ip_address;
    return;
  }

  if (node_state->am_master) {
    // TODO(simon): signal other controllers here
    node_state->am_master = false;
    m_connector.Disconnect(
        IPV4SocketAddress(ip_address, ola::plugin::e131::E133_PORT));
  } else {
    // we lost the race, so don't try to reconnect
    m_connector.Disconnect(
        IPV4SocketAddress(ip_address, ola::plugin::e131::E133_PORT), true);
  }

  delete node_state->health_checked_connection;
  node_state->health_checked_connection = NULL;

  delete node_state->out_transport;
  node_state->out_transport = NULL;

  delete node_state->in_transport;
  node_state->in_transport = NULL;

  m_ss.RemoveReadDescriptor(node_state->socket.get());

  // Terminate for now
  m_ss.Terminate();
}


/**
 * Called when we receive E1.33 data. If this arrived over TCP we notify the
 * health checked connection.
 */
void SimpleE133Monitor::RLPDataReceived(
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

  node_state->socket->AssociateSelectServer(&m_ss);
  OutgoingStreamTransport *outgoing_transport = new OutgoingStreamTransport(
      node_state->socket.get());

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
  auto_ptr<RDMCommand> command(
      RDMCommand::Inflate(reinterpret_cast<const uint8_t*>(raw_request.data()),
                          raw_request.size()));
  if (command.get()) {
    m_command_printer.Print(command.get());
  } else {
    ola::FormatData(&cout, rdm_data, slot_count, 2);
  }
}


/*
 * Startup a node
 */
int main(int argc, char *argv[]) {
  options opts;
  opts.pid_location = PID_DATA_DIR;
  opts.log_level = ola::OLA_LOG_WARN;
  opts.help = false;
  opts.use_openslp = false;
  ParseOptions(argc, argv, &opts);
  PidStoreHelper pid_helper(opts.pid_location, 4);

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

  SimpleE133Monitor::SLPOption slp_option = SimpleE133Monitor::NO_SLP;
  if (targets.empty()) {
    slp_option = opts.use_openslp ? SimpleE133Monitor::OPEN_SLP :
        SimpleE133Monitor::OLA_SLP;
  }
  SimpleE133Monitor monitor(&pid_helper, slp_option);
  if (!monitor.Init())
    exit(EX_UNAVAILABLE);

  if (!targets.empty()) {
    // manually add the responder IPs
    vector<IPV4Address>::const_iterator iter = targets.begin();
    for (; iter != targets.end(); ++iter)
      monitor.AddIP(*iter);
  }
  monitor.Run();
}
