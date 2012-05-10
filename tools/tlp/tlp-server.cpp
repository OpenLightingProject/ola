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
 * tlp-server.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <sysexits.h>
#include <termios.h>

#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/Logging.h>
#include <ola/io/SelectServer.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/InterfacePicker.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/Socket.h>
#include <ola/network/TCPSocketFactory.h>
#include <ola/rdm/UID.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "plugins/e131/e131/BaseInflator.h"
#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/RootInflator.h"
#include "plugins/e131/e131/HeaderSet.h"
#include "plugins/e131/e131/RootSender.h"
#include "plugins/e131/e131/UDPTransport.h"


using ola::NewCallback;
using ola::NewSingleCallback;
using ola::TimeInterval;
using ola::TimeStamp;
using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using ola::network::NetworkToHost;
using ola::network::TcpAcceptingSocket;
using ola::network::TcpSocket;
using ola::network::UdpSocket;
using ola::plugin::e131::HeaderSet;
using ola::rdm::UID;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::vector;

static const uint16_t TLP_ADVERTISMENT_PERIOD = 30;  // 30 s
static const uint16_t TLP_NODE_LIST_PRUNE_INTERVAL = 10;  // 10s
static const uint16_t TLP_PORT = 5570;
static const uint32_t TLP_REGISTER_VECTOR = 42;
static const uint32_t TLP_REGISTER_ACK_VECTOR = 43;
static const uint32_t TLP_REGISTRY_ADVERT_VECTOR = 44;


typedef struct {
  bool help;
  string ip_address;
  ola::log_level log_level;
} options;


/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {"ip", required_argument, 0, 'i'},
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "hi:l:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 'h':
        opts->help = true;
        break;
      case 'i':
        opts->ip_address = optarg;
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
  "Run the TLP server.\n"
  "\n"
  "  -h, --help                Display this help message and exit.\n"
  "  -i, --ip                  The IP address to listen on.\n"
  "  -l, --log-level <level>   Set the logging level 0 .. 4.\n"
  << endl;
  exit(0);
}


/*
 * A RootInflator that just passes the PDU data & vector to a callback
 */
class HackyRootInflator: public ola::plugin::e131::RootInflator {
  public:
    typedef ola::Callback4<void,
                           const IPV4Address&,
                           uint32_t,
                           const uint8_t*,
                           unsigned int> OnData;
    explicit HackyRootInflator(OnData *on_data)
      : RootInflator(NULL),
        m_callback(on_data) {
    }

  protected:
    bool HandlePDUData(uint32_t vector,
                       HeaderSet &header,
                       const uint8_t *data,
                       unsigned int pdu_len) {
      m_callback->Run(header.GetTransportHeader().SourceIP(),
                      vector,
                      data,
                      pdu_len);
      return true;
    }

  private:
    auto_ptr<OnData> m_callback;
};


/**
 * An entry in the node list
 */
class NodeEntry {
  public:
    NodeEntry(const IPV4Address &ip, const UID &uid, const TimeStamp &expiry)
      : ip(ip),
        uid(uid),
        expiry(expiry) {
    }

  IPV4Address ip;
  UID uid;
  TimeStamp expiry;
};


/**
 * A trival location protocol server.
 */
class TLPServer {
  public:
    explicit TLPServer(const IPV4Address &iface_ip);
    ~TLPServer();

    bool Init();
    void Run() { m_ss.Run(); }
    void Stop() { m_ss.Terminate(); }

  private:
    typedef vector<TcpSocket*> TCPSocketList;
    typedef vector<NodeEntry*> NodeList;

    ola::io::SelectServer m_ss;
    ola::network::TCPSocketFactory m_tcp_socket_factory;
    TcpAcceptingSocket m_tcp_accept_socket;
    TCPSocketList m_tcp_sockets;
    IPV4Address m_multicast_address;
    const IPV4Address m_iface_address;

    UdpSocket m_udp_socket;
    ola::io::UnmanagedFileDescriptor m_stdin_descriptor;
    termios m_old_tc;

    // the nodes
    NodeList m_nodes;

    // The Controller's CID
    ola::plugin::e131::CID m_cid;

    // senders
    ola::plugin::e131::RootSender m_root_sender;

    // inflators
    HackyRootInflator m_root_inflator;

    // transports
    ola::plugin::e131::IncomingUDPTransport m_incoming_udp_transport;
    ola::plugin::e131::OutgoingUDPTransportImpl m_outgoing_udp_transport;

    // tcp methods
    void NewTCPConnection(TcpSocket *socket);
    void ReceiveTCPData(TcpSocket *socket);
    void SocketClosed(TcpSocket *socket);
    void SendJoinUpdateToClients(const IPV4Address &address, const UID &uid);
    void SendPartUpdateToClients(const IPV4Address &address, const UID &uid);
    void SendStringToClients(const string &output);
    void SendState(TcpSocket *socket);

    // stdin
    void Input();

    // housekeeping methods
    bool SendPeriodicAdvert();
    bool LookForStaleEntries();

    // udp methods
    void UDPMessage(const IPV4Address &ip,
                    uint32_t vector,
                    const uint8_t *data,
                    unsigned int length);
    void CreateNodeEntry(const IPV4Address &ip,
                         const UID &uid,
                         uint16_t lifetime);
};



/**
 * Setup a new TLP server
 * @param iface_address the IPv4 address of the interface we should use.
 */
TLPServer::TLPServer(const IPV4Address &iface_address)
    : m_tcp_socket_factory(NewCallback(this, &TLPServer::NewTCPConnection)),
      m_tcp_accept_socket(&m_tcp_socket_factory),
      m_iface_address(iface_address),
      m_stdin_descriptor(STDIN_FILENO),
      m_cid(ola::plugin::e131::CID::Generate()),
      m_root_sender(m_cid),
      m_root_inflator(NewCallback(this, &TLPServer::UDPMessage)),
      m_incoming_udp_transport(&m_udp_socket, &m_root_inflator),
      m_outgoing_udp_transport(&m_udp_socket) {
  m_multicast_address = IPV4Address(
      HostToNetwork(239U << 24 |
                    255U << 16 |
                    255u << 8 |
                    238));
}


TLPServer::~TLPServer() {
  // close out all tcp sockets
  TCPSocketList::iterator iter = m_tcp_sockets.begin();
  for (; iter != m_tcp_sockets.end(); ++iter) {
    (*iter)->Close();
    delete *iter;
  }
  m_tcp_sockets.clear();

  // clean node list
  NodeList::iterator node_iter = m_nodes.begin();
  for (; node_iter != m_nodes.end(); ++node_iter) {
    delete *node_iter;
  }
  m_nodes.clear();

  m_udp_socket.Close();
  m_tcp_accept_socket.Close();
  tcsetattr(STDIN_FILENO, TCSANOW, &m_old_tc);
}


/**
 * Init the server
 */
bool TLPServer::Init() {
  OLA_INFO << "Interface address is " << m_iface_address;

  // setup notifications for stdin & turn off buffering
  m_stdin_descriptor.SetOnData(ola::NewCallback(this, &TLPServer::Input));
  m_ss.AddReadDescriptor(&m_stdin_descriptor);
  tcgetattr(STDIN_FILENO, &m_old_tc);
  termios new_tc = m_old_tc;
  new_tc.c_lflag &= static_cast<tcflag_t>(~ICANON & ~ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tc);

  // setup the accepting TCP socket
  if (!m_tcp_accept_socket.Listen(m_iface_address, TLP_PORT)) {
    return false;
  }

  m_ss.AddReadDescriptor(&m_tcp_accept_socket);

  // setup the UDP socket
  if (!m_udp_socket.Init()) {
    m_tcp_accept_socket.Close();
    return false;
  }

  if (!m_udp_socket.Bind(TLP_PORT)) {
    m_tcp_accept_socket.Close();
    return false;
  }

  m_udp_socket.SetMulticastInterface(m_iface_address);

  // join the multicast group
  if (!m_udp_socket.JoinMulticast(m_iface_address, m_multicast_address)) {
    m_tcp_accept_socket.Close();
    return false;
  }

  m_udp_socket.SetOnData(
        NewCallback(&m_incoming_udp_transport,
                    &ola::plugin::e131::IncomingUDPTransport::Receive));

  m_ss.AddReadDescriptor(&m_udp_socket);

  // setup the advertisment timeout, 30s
  m_ss.RegisterRepeatingTimeout(
      TLP_ADVERTISMENT_PERIOD * 1000,
      NewCallback(this, &TLPServer::SendPeriodicAdvert));

  // check for stale entries every 5s
  m_ss.RegisterRepeatingTimeout(
      TLP_NODE_LIST_PRUNE_INTERVAL * 1000,
      NewCallback(this, &TLPServer::LookForStaleEntries));

  // send one now
  SendPeriodicAdvert();
  return true;
}


/**
 * Called when a TCP socket is connected.
 */
void TLPServer::NewTCPConnection(TcpSocket *socket) {
  IPV4Address peer_address;
  uint16_t port;
  socket->GetPeer(&peer_address, &port);
  OLA_INFO << "New connection from " << peer_address << ":" << port;

  socket->SetOnData(
      NewCallback(this, &TLPServer::ReceiveTCPData, socket));
  socket->SetOnClose(
    NewSingleCallback(this, &TLPServer::SocketClosed, socket));
  m_ss.AddReadDescriptor(socket);
  m_tcp_sockets.push_back(socket);
}


/**
 * Receive data on a TCP connection
 */
void TLPServer::ReceiveTCPData(TcpSocket *socket) {
  uint8_t trash[512];
  unsigned int data_length;
  int ok = socket->Receive(trash, sizeof(trash), data_length);
  if (ok == 0)  {
    // OLA_INFO << "got " << data_length << " on a tcp connection";
    // ola::FormatData(&cout, trash, data_length);
    for (unsigned int i = 0; i < data_length; ++i) {
      if (trash[i] == 'g') {
        OLA_INFO << "Sending state";
        SendState(socket);
      }
    }
  }
}


/**
 * Called when a socket is closed.
 */
void TLPServer::SocketClosed(TcpSocket *socket) {
  auto_ptr<TcpSocket> socket_to_delete(socket);
  OLA_INFO << "closing TCP socket";
  m_ss.RemoveReadDescriptor(socket);

  TCPSocketList::iterator iter = m_tcp_sockets.begin();
  for (; iter != m_tcp_sockets.end(); ++iter) {
    if (*iter == socket)
      break;
  }

  if (iter == m_tcp_sockets.end()) {
    OLA_FATAL << "Unable to locate socket for " << socket;
    return;
  }

  m_tcp_sockets.erase(iter);
}


/**
 * Send a join message to all connected clients
 */
void TLPServer::SendJoinUpdateToClients(const IPV4Address &address,
                                        const UID &uid) {
  stringstream str;
  str << "Join: " << address << ", " << uid << endl;
  SendStringToClients(str.str());
}


/**
 * Send a part message to all connected clients
 */
void TLPServer::SendPartUpdateToClients(const IPV4Address &address,
                                        const UID &uid) {
  stringstream str;
  str << "Part: " << address << ", " << uid << endl;
  SendStringToClients(str.str());
}


/**
 * Send a string to all clients
 */
void TLPServer::SendStringToClients(const string &output) {
  TCPSocketList::iterator iter = m_tcp_sockets.begin();
  for (; iter != m_tcp_sockets.end(); ++iter) {
    (*iter)->Send(reinterpret_cast<const uint8_t*>(output.data()),
                  output.size());
  }
  OLA_INFO << "Sent \"" << output.substr(0, output.size() - 1) << "\" to " <<
    m_tcp_sockets.size() << " clients";
}


/**
 * Send a full state message to a single client
 */
void TLPServer::SendState(TcpSocket *socket) {
  NodeList::iterator iter = m_nodes.begin();
  for ( ; iter != m_nodes.end(); ++iter) {
    stringstream str;
    str << "Active: " << (*iter)->ip << ", " << (*iter)->uid << endl;
    string output = str.str();
    socket->Send(reinterpret_cast<const uint8_t*>(output.data()),
                 output.size());
  }
}


/**
 * Called when there is data on stdin.
 */
void TLPServer::Input() {
  IPV4Address ip_address;
  ip_address.FromString("127.0.0.1", &ip_address);
  UID fake_uid(0x7a80, 01);

  switch (getchar()) {
    case 'c':
      CreateNodeEntry(ip_address, fake_uid, 10);
      break;
    case 'j':
      SendJoinUpdateToClients(ip_address, fake_uid);
      break;
    case 'q':
      m_ss.Terminate();
      break;
    case 'p':
      SendPartUpdateToClients(ip_address, fake_uid);
      break;
    default:
      break;
  }
}


/**
 * Send the periodic advertisment.
 */
bool TLPServer::SendPeriodicAdvert() {
  OLA_INFO << "Sending advert";

  ola::plugin::e131::OutgoingUDPTransport transport(&m_outgoing_udp_transport,
                                                    m_multicast_address,
                                                    TLP_PORT);

  bool ok = m_root_sender.SendEmpty(TLP_REGISTRY_ADVERT_VECTOR, &transport);
  if (!ok)
    OLA_WARN << "Failed to send Advert";
  return true;
}


/**
 * Create a node entry
 */
void TLPServer::CreateNodeEntry(const IPV4Address &ip,
                                const UID &uid,
                                uint16_t lifetime) {
  const TimeStamp *now = m_ss.WakeUpTime();

  NodeList::iterator iter = m_nodes.begin();
  for (; iter != m_nodes.end(); ++iter) {
    if ((*iter)->ip == ip && (*iter)->uid == uid)
      break;
  }

  TimeStamp expiry = *now + TimeInterval(lifetime, 0);
  if (iter == m_nodes.end()) {
    OLA_INFO << "creating " << ip << ", " << uid << ", " << lifetime;
    NodeEntry *node = new NodeEntry(ip, uid, expiry);
    m_nodes.push_back(node);
    SendJoinUpdateToClients(ip, uid);
  } else {
    OLA_INFO << "updating " << ip << ", expires in " << lifetime << " seconds";
    (*iter)->expiry = expiry;
  }
}


/**
 * Walk the list looking for stale entries
 */
bool TLPServer::LookForStaleEntries() {
  OLA_INFO << "looking for stale entries";
  const TimeStamp now = *(m_ss.WakeUpTime());

  NodeList::iterator node_iter = m_nodes.begin();
  while (node_iter != m_nodes.end()) {
    if ((*node_iter)->expiry < now) {
      OLA_INFO << "Node has expired " << (*node_iter)->ip << ", " <<
        (*node_iter)->uid;
      SendPartUpdateToClients((*node_iter)->ip, (*node_iter)->uid);
      delete *node_iter;
      node_iter = m_nodes.erase(node_iter);
    } else {
      node_iter++;
    }
  }
  return true;
}


/**
 * Called when we get a UDP message
 */
void TLPServer::UDPMessage(const IPV4Address &ip,
                           uint32_t vector,
                           const uint8_t *data,
                           unsigned int length) {
  OLA_INFO << "got udp message from " << ip << ", vector is " << vector;

  if (vector == TLP_REGISTER_VECTOR) {
    if (length != 8) {
      OLA_WARN << "data length wasn't 8!";
      return;
    }

    // extract the uid & lifetime
    UID client_uid(data);
    uint16_t lifetime;
    memcpy(reinterpret_cast<void*>(&lifetime), data + 6, 2);
    lifetime = NetworkToHost(lifetime);
    OLA_INFO << "UID " << client_uid << ", lifetime " << lifetime;

    CreateNodeEntry(ip, client_uid, lifetime);

    // send an ack
    ola::plugin::e131::OutgoingUDPTransport transport(
        &m_outgoing_udp_transport,
        ip,
        TLP_PORT);

    bool ok = m_root_sender.SendEmpty(TLP_REGISTER_ACK_VECTOR, &transport);
    if (!ok)
      OLA_WARN << "Failed to send ack";
  } else {
    OLA_INFO << "Got message with unknown vector " << vector;
  }
}


TLPServer *server;

static void InteruptSignal(int signo) {
  if (server)
    server->Stop();
  (void) signo;
}


/*
 * Startup the server.
 */
int main(int argc, char *argv[]) {
  options opts;
  opts.log_level = ola::OLA_LOG_WARN;
  opts.help = false;
  ParseOptions(argc, argv, &opts);

  if (opts.help)
    DisplayHelpAndExit(argv);

  ola::InitLogging(opts.log_level, ola::OLA_LOG_STDERR);

  // find an interface to use
  ola::network::Interface interface;

  {
    auto_ptr<const ola::network::InterfacePicker> picker(
      ola::network::InterfacePicker::NewPicker());
    if (!picker->ChooseInterface(&interface, opts.ip_address)) {
      OLA_INFO << "Failed to find an interface";
      exit(EX_UNAVAILABLE);
    }
  }

  TLPServer *server = new TLPServer(interface.ip_address);
  if (!server->Init())
    exit(EX_UNAVAILABLE);

  // signal handler
  struct sigaction act, oact;
  act.sa_handler = InteruptSignal;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if (sigaction(SIGINT, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGINT";
    return false;
  }

  cout << "---------------  Controls  ----------------\n";
  cout << " c - Create a fake node\n";
  cout << " j - Send a fake join\n";
  cout << " p - Send a fake part\n";
  cout << " q - Quit\n";
  cout << "-------------------------------------------\n";
  server->Run();
  delete server;
}
