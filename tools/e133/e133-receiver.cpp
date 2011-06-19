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
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <sysexits.h>

#include <ola/BaseTypes.h>
#include <ola/Logging.h>
#include <ola/network/SelectServer.h>
#include <ola/rdm/UID.h>

#include <string>
#include <iostream>

#include "plugins/dummy/DummyResponder.h"
#include "plugins/e131/e131/ACNPort.h"

#include "tools/e133/E133Node.h"
#include "tools/e133/E133Receiver.h"
#include "tools/e133/SlpThread.h"

using std::string;
using ola::rdm::UID;

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
  "  -l, --log-level <level>   Set the loggging level 0 .. 4.\n"
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
    explicit SimpleE133Node(const options &opts);
    ~SimpleE133Node();

    bool Init();
    void Run();
    void Stop() { m_ss.Terminate(); }

  private:
    ola::network::SelectServer m_ss;
    SlpThread m_slp_thread;
    E133Node m_e133_node;
    ola::plugin::dummy::DummyResponder m_responder;
    E133Receiver m_receiver;
    uint16_t m_lifetime;
    UID m_uid;
    string m_service_name;

    SimpleE133Node(const SimpleE133Node&);
    SimpleE133Node operator=(const SimpleE133Node&);

    void RegisterCallback(bool ok);
    void DeRegisterCallback(bool ok);
};


/**
 * Constructor
 */
SimpleE133Node::SimpleE133Node(const options &opts)
    : m_slp_thread(&m_ss),
      m_e133_node(&m_ss, opts.ip_address, ola::plugin::e131::ACN_PORT),
      m_responder(*opts.uid),
      m_receiver(opts.universe, &m_responder),
      m_lifetime(opts.lifetime),
      m_uid(*opts.uid) {
}


SimpleE133Node::~SimpleE133Node() {
  m_slp_thread.Join();
  m_slp_thread.Cleanup();
}


/**
 * Init this node
 */
bool SimpleE133Node::Init() {
  if (!m_e133_node.Init()) {
    return false;
  }
  m_e133_node.RegisterComponent(&m_receiver);

  std::stringstream str;
  str << m_e133_node.V4Address() << ":" << ola::plugin::e131::ACN_PORT << "/"
    << std::setfill('0') << std::setw(4) << std::hex
    << m_uid.ManufacturerId() << std::setw(8) << m_uid.DeviceId();
  m_service_name = str.str();
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
  m_ss.Restart();
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


SimpleE133Node *simple_node;

/*
 * Terminate cleanly on interrupt.
 */
static void InteruptSignal(int signo) {
  signo = 0;
  if (simple_node)
    simple_node->Stop();
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

  SimpleE133Node node(opts);
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

  node.Run();

  delete opts.uid;
}
