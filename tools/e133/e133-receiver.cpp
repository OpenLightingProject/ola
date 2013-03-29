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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <termios.h>

#include <ola/BaseTypes.h>
#include <ola/Logging.h>
#include <ola/io/SelectServer.h>
#include <ola/network/InterfacePicker.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/UID.h>

#include <iostream>
#include <memory>
#include <string>

#include "plugins/dummy/DummyResponder.h"
#include "plugins/e131/e131/ACNPort.h"

#include "tools/e133/E133Device.h"
#include "tools/e133/EndpointManager.h"
#include "tools/e133/OLASLPThread.h"
#ifdef HAVE_LIBSLP
#include "tools/e133/OpenSLPThread.h"
#endif
#include "tools/e133/RootEndpoint.h"
#include "tools/e133/TCPConnectionStats.h"

using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;


typedef struct {
  bool help;
  bool use_openslp;
  ola::log_level log_level;
  unsigned int universe;
  string ip_address;
  uint16_t lifetime;
  auto_ptr<UID> uid;
} options;


/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  enum {
    OPENSLP_OPTION = 256,
  };

  int uid_set = 0;
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"ip", required_argument, 0, 'i'},
      {"log-level", required_argument, 0, 'l'},
      {"timeout", required_argument, 0, 't'},
      {"uid", required_argument, &uid_set, 1},
      {"universe", required_argument, 0, 'u'},
#ifdef HAVE_LIBSLP
      {"openslp", no_argument, 0, OPENSLP_OPTION},
#endif
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "hi:l:t:u:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        if (uid_set)
          opts->uid.reset(UID::FromString(optarg));
        break;
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
      case 't':
        opts->lifetime = atoi(optarg);
        break;
      case 'u':
        opts->universe = atoi(optarg);
        break;
      case OPENSLP_OPTION:
        opts->use_openslp = true;
        break;
      case '?':
        break;
      default:
       break;
    }
  }
  return;
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char *argv[]) {
  cout << "Usage: " << argv[0] << " [options]\n"
  "\n"
  "Run a very simple E1.33 Responder.\n"
  "\n"
  "  -h, --help                Display this help message and exit.\n"
  "  -i, --ip                  The IP address to listen on.\n"
  "  -l, --log-level <level>   Set the logging level 0 .. 4.\n"
  "  -t, --timeout <seconds>   The value to use for the service lifetime\n"
  "  -u, --universe <universe> The universe to respond on (> 0).\n"
  "  --uid <uid>               The UID of the responder.\n"
#ifdef HAVE_LIBSLP
  "  --openslp                 Use openslp rather than the OLA SLP server\n"
#endif
  << endl;
  exit(0);
}


/**
 * A very simple E1.33 node that registers itself using SLP and responds to
 * messages.
 */
class SimpleE133Node {
  public:
    explicit SimpleE133Node(const IPV4Address &ip_address,
                            const options &opts);
    ~SimpleE133Node();

    bool Init();
    void Run();
    void Stop() { m_ss.Terminate(); }

  private:
    ola::io::SelectServer m_ss;
    ola::io::UnmanagedFileDescriptor m_stdin_descriptor;
    auto_ptr<BaseSLPThread> m_slp_thread;
    EndpointManager m_endpoint_manager;
    TCPConnectionStats m_tcp_stats;
    E133Device m_e133_device;
    RootEndpoint m_root_endpoint;
    E133Endpoint m_first_endpoint;
    ola::plugin::dummy::DummyResponder m_responder;
    uint16_t m_lifetime;
    UID m_uid;
    const IPV4Address m_ip_address;
    termios m_old_tc;

    SimpleE133Node(const SimpleE133Node&);
    SimpleE133Node operator=(const SimpleE133Node&);

    void RegisterCallback(bool ok);
    void DeRegisterCallback(bool ok);

    void Input();
    void DumpTCPStats();
    void SendUnsolicited();
};


/**
 * Constructor
 */
SimpleE133Node::SimpleE133Node(const IPV4Address &ip_address,
                               const options &opts)
    : m_stdin_descriptor(STDIN_FILENO),
      m_e133_device(&m_ss, ip_address, &m_endpoint_manager, &m_tcp_stats),
      m_root_endpoint(*opts.uid, &m_endpoint_manager, &m_tcp_stats),
      m_first_endpoint(NULL),  // NO CONTROLLER FOR NOW!
      m_responder(*opts.uid),
      m_lifetime(opts.lifetime),
      m_uid(*opts.uid),
      m_ip_address(ip_address) {
  if (opts.use_openslp) {
#ifdef HAVE_LIBSLP
    m_slp_thread.reset(new OpenSLPThread(&m_ss));
#else
    OLA_WARN << "openslp not installed";
#endif
  } else {
    m_slp_thread.reset(new OLASLPThread(&m_ss));
  }
}


SimpleE133Node::~SimpleE133Node() {
  m_endpoint_manager.UnRegisterEndpoint(1);
  tcsetattr(STDIN_FILENO, TCSANOW, &m_old_tc);
  m_slp_thread->Join(NULL);
  m_slp_thread->Cleanup();
}


/**
 * Init this node
 */
bool SimpleE133Node::Init() {
  // setup notifications for stdin & turn off buffering
  m_stdin_descriptor.SetOnData(ola::NewCallback(this, &SimpleE133Node::Input));
  m_ss.AddReadDescriptor(&m_stdin_descriptor);
  tcgetattr(STDIN_FILENO, &m_old_tc);
  termios new_tc = m_old_tc;
  new_tc.c_lflag &= static_cast<tcflag_t>(~ICANON & ~ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tc);

  if (!m_e133_device.Init())
    return false;

  // register the root endpoint
  m_e133_device.SetRootEndpoint(&m_root_endpoint);
  // add a single endpoint
  m_endpoint_manager.RegisterEndpoint(1, &m_first_endpoint);

  // Start the SLP thread.
  if (!m_slp_thread->Init()) {
    OLA_WARN << "SLPThread Init() failed";
    return false;
  }
  m_slp_thread->Start();
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
void SimpleE133Node::Input() {
  switch (getchar()) {
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
  cout << "IP: " << m_tcp_stats.ip_address << endl;
  cout << "Connection Unhealthy Events: " << m_tcp_stats.unhealthy_events <<
    endl;
  cout << "Connection Events: " << m_tcp_stats.connection_events << endl;
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

  tcp_stats_message.ip_address = m_tcp_stats.ip_address.AsInt();
  tcp_stats_message.unhealthy_events =
    HostToNetwork(m_tcp_stats.unhealthy_events);
  tcp_stats_message.connection_events =
    HostToNetwork(m_tcp_stats.connection_events);

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
  options opts;
  opts.use_openslp = false;
  opts.log_level = ola::OLA_LOG_WARN;
  opts.lifetime = 300;  // 5 mins is a good compromise
  opts.universe = 1;
  opts.help = false;
  ParseOptions(argc, argv, &opts);

  if (opts.help)
    DisplayHelpAndExit(argv);

  ola::InitLogging(opts.log_level, ola::OLA_LOG_STDERR);
  if (!opts.uid.get()) {
    opts.uid.reset(new ola::rdm::UID(OPEN_LIGHTING_ESTA_CODE, 0xffffff00));
  }

  ola::network::Interface interface;

  {
    auto_ptr<const ola::network::InterfacePicker> picker(
      ola::network::InterfacePicker::NewPicker());
    if (!picker->ChooseInterface(&interface, opts.ip_address)) {
      OLA_INFO << "Failed to find an interface";
      exit(EX_UNAVAILABLE);
    }
  }

  SimpleE133Node node(interface.ip_address, opts);
  simple_node = &node;

  if (!node.Init())
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
  cout << " c - Close the TCP connection\n";
  cout << " q - Quit\n";
  cout << " s - Send Status Message\n";
  cout << " t - Dump TCP stats\n";
  cout << "-------------------------------------------\n";

  node.Run();
}
