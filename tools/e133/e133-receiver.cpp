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
 * e133-receiver.cpp
 * Copyright (C) 2011 Simon Newton
 *
 * This creates a E1.33 receiver with one (emulated) RDM responder. The node is
 * registered in slp and the RDM responder responds to E1.33 commands.
 *
 * TODO(simon): Implement the node management commands.
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <termios.h>

#include <ola/BaseTypes.h>
#include <ola/Logging.h>
#include <ola/network/SelectServer.h>
#include <ola/network/InterfacePicker.h>
#include <ola/rdm/UID.h>

#include <iostream>
#include <memory>
#include <string>

#include "plugins/dummy/DummyResponder.h"
#include "plugins/e131/e131/ACNPort.h"

#include "tools/e133/E133Device.h"
#include "tools/e133/EndpointManager.h"
#include "tools/e133/RootEndpoint.h"
#include "tools/e133/SlpThread.h"
#include "tools/e133/TCPConnectionStats.h"

using std::string;
using ola::rdm::UID;
using std::auto_ptr;
using ola::network::IPV4Address;


typedef struct {
  bool help;
  ola::log_level log_level;
  unsigned int universe;
  string ip_address;
  uint16_t lifetime;
  UID *uid;
} options;


/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  int uid_set = 0;
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"ip", required_argument, 0, 'i'},
      {"log-level", required_argument, 0, 'l'},
      {"timeout", required_argument, 0, 't'},
      {"uid", required_argument, &uid_set, 1},
      {"universe", required_argument, 0, 'u'},
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
          opts->uid = UID::FromString(optarg);
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
  std::cout << "Usage: " << argv[0] << " [options]\n"
  "\n"
  "Run a very simple E1.33 Responder.\n"
  "\n"
  "  -h, --help                Display this help message and exit.\n"
  "  -i, --ip                  The IP address to listen on.\n"
  "  -l, --log-level <level>   Set the logging level 0 .. 4.\n"
  "  -t, --timeout <seconds>   The value to use for the service lifetime\n"
  "  -u, --universe <universe> The universe to respond on (> 0).\n"
  "  --uid <uid>               The UID of the responder.\n"
  << std::endl;
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
    ola::network::SelectServer m_ss;
    ola::network::UnmanagedFileDescriptor m_stdin_descriptor;
    SlpThread m_slp_thread;
    EndpointManager m_endpoint_manager;
    TCPConnectionStats m_tcp_stats;
    E133Device m_e133_device;
    RootEndpoint m_root_endpoint;
    E133Endpoint m_first_endpoint;
    ola::plugin::dummy::DummyResponder m_responder;
    uint16_t m_lifetime;
    UID m_uid;
    string m_service_name;
    termios m_old_tc;

    SimpleE133Node(const SimpleE133Node&);
    SimpleE133Node operator=(const SimpleE133Node&);

    void RegisterCallback(bool ok);
    void DeRegisterCallback(bool ok);

    void Input();
    void DumpTCPStats();
};


/**
 * Constructor
 */
SimpleE133Node::SimpleE133Node(const IPV4Address &ip_address,
                               const options &opts)
    : m_stdin_descriptor(STDIN_FILENO),
      m_slp_thread(&m_ss),
      m_e133_device(&m_ss, ip_address, &m_endpoint_manager, &m_tcp_stats),
      m_root_endpoint(*opts.uid, &m_endpoint_manager, &m_tcp_stats),
      m_first_endpoint(NULL),  // NO CONTROLLER FOR NOW!
      m_responder(*opts.uid),
      m_lifetime(opts.lifetime),
      m_uid(*opts.uid) {
  std::stringstream str;
  str << ip_address << ":" << ola::plugin::e131::ACN_PORT << "/"
    << std::setfill('0') << std::setw(4) << std::hex
    << m_uid.ManufacturerId() << std::setw(8) << m_uid.DeviceId();
  m_service_name = str.str();
}


SimpleE133Node::~SimpleE133Node() {
  m_endpoint_manager.UnRegisterEndpoint(1);
  tcsetattr(STDIN_FILENO, TCSANOW, &m_old_tc);
  m_slp_thread.Join();
  m_slp_thread.Cleanup();
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

  if (!m_e133_device.Init()) {
    return false;
  }

  // register the root endpoint
  m_e133_device.SetRootEndpoint(&m_root_endpoint);
  // add a single endpoint
  m_endpoint_manager.RegisterEndpoint(1, &m_first_endpoint);

  // register in SLP
  OLA_INFO << "service is " << m_service_name;
  if (!m_slp_thread.Init()) {
    OLA_WARN << "SlpThread Init() failed";
    return false;
  }

  m_slp_thread.Start();
  return true;
}


void SimpleE133Node::Run() {
  m_slp_thread.Register(
    ola::NewSingleCallback(this, &SimpleE133Node::RegisterCallback),
    m_service_name,
    m_lifetime);

  m_ss.Run();
  OLA_INFO << "Starting shutdown process";

  m_slp_thread.DeRegister(
    ola::NewSingleCallback(this, &SimpleE133Node::DeRegisterCallback),
    m_service_name);
  m_ss.Run();
}


/**
 * Called when a registration request completes.
 */
void SimpleE133Node::RegisterCallback(bool ok) {
  OLA_INFO << "in register callback, state is " << ok;
}


/**
 * Called when a de-registration request completes.
 */
void SimpleE133Node::DeRegisterCallback(bool ok) {
  OLA_INFO << "in deregister callback, state is " << ok;
  m_ss.Terminate();
}


/**
 * Called when there is data on stdin.
 */
void SimpleE133Node::Input() {
  switch (getchar()) {
    case 'q':
      m_ss.Terminate();
      break;
    case 's':
      OLA_INFO << "This would send a unsolicited message";
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
  OLA_INFO << "IP: " << m_tcp_stats.ip_address;
  OLA_INFO << "Connection Unhealthy Events: " << m_tcp_stats.unhealthy_events;
  OLA_INFO << "Connection Events: " << m_tcp_stats.connection_events;
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
  opts.log_level = ola::OLA_LOG_WARN;
  opts.lifetime = 300;  // 5 mins is a good compromise
  opts.universe = 1;
  opts.help = false;
  opts.uid = NULL;
  ParseOptions(argc, argv, &opts);

  if (opts.help)
    DisplayHelpAndExit(argv);

  ola::InitLogging(opts.log_level, ola::OLA_LOG_STDERR);
  if (!opts.uid) {
    opts.uid = new ola::rdm::UID(OPEN_LIGHTING_ESTA_CODE, 0xffffff00);
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
    delete opts.uid;
    return false;
  }

  OLA_INFO << "---------------  Controls  ----------------";
  OLA_INFO << " q - Quit";
  OLA_INFO << " s - Send Status Message";
  OLA_INFO << " t - Dump TCP stats";
  OLA_INFO << "-------------------------------------------";

  node.Run();

  delete opts.uid;
}
