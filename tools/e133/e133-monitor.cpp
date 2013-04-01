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

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <errno.h>
#include <getopt.h>
#include <sysexits.h>

#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/io/SelectServer.h>
#include <ola/io/StdinHandler.h>
#include <ola/network/IPV4Address.h>
#include <ola/rdm/CommandPrinter.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/UID.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "plugins/e131/e131/CID.h"

#include "tools/e133/DeviceManager.h"
#include "tools/e133/E133URLParser.h"
#include "tools/e133/OLASLPThread.h"
#include "tools/e133/MessageBuilder.h"
#include "tools/e133/SLPThread.h"
#include "tools/slp/URLEntry.h"

#ifdef HAVE_LIBSLP
#include "tools/e133/OpenSLPThread.h"
#endif

using ola::NewCallback;
using ola::network::IPV4Address;
using ola::rdm::PidStoreHelper;
using ola::rdm::RDMCommand;
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
    ola::io::StdinHandler m_stdin_handler;
    auto_ptr<BaseSLPThread> m_slp_thread;

    MessageBuilder m_message_builder;
    DeviceManager m_device_manager;

    void Input(char c);
    void DiscoveryCallback(bool status, const URLEntries &urls);

    bool EndpointRequest(
        const ola::plugin::e131::TransportHeader &transport_header,
        const ola::plugin::e131::E133Header &e133_header,
        const string &raw_request);
};


/**
 * Setup a new Monitor
 */
SimpleE133Monitor::SimpleE133Monitor(
    PidStoreHelper *pid_helper,
    SLPOption slp_option)
    : m_command_printer(&cout, pid_helper),
      m_stdin_handler(&m_ss,
                      ola::NewCallback(this, &SimpleE133Monitor::Input)),
      m_message_builder(ola::plugin::e131::CID::Generate(), "OLA Monitor"),
      m_device_manager(&m_ss, &m_message_builder) {
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

  m_device_manager.SetRDMMessageCallback(
      NewCallback(this, &SimpleE133Monitor::EndpointRequest));

  // TODO(simon): add a controller discovery callback here as well.
}


SimpleE133Monitor::~SimpleE133Monitor() {
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
  m_device_manager.AddDevice(ip_address);
}


void SimpleE133Monitor::Input(char c) {
  switch (c) {
    case 'q':
      m_ss.Terminate();
      break;
    default:
      break;
  }
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
 * We received data to endpoint 0
 */
bool SimpleE133Monitor::EndpointRequest(
    const ola::plugin::e131::TransportHeader &transport_header,
    const ola::plugin::e131::E133Header&,
    const string &raw_request) {
  unsigned int slot_count = raw_request.size();
  const uint8_t *rdm_data = reinterpret_cast<const uint8_t*>(
    raw_request.data());

  cout << "From " << transport_header.Source() << ":" << endl;
  auto_ptr<RDMCommand> command(
      RDMCommand::Inflate(reinterpret_cast<const uint8_t*>(raw_request.data()),
                          raw_request.size()));
  if (command.get()) {
    command->Print(&m_command_printer, false, true);
  } else {
    ola::FormatData(&cout, rdm_data, slot_count, 2);
  }
  return true;
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
