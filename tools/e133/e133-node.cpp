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
 * e133-node.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/InterfacePicker.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/SelectServer.h>
#include <ola/rdm/RDMCommand.h>

#include <iostream>
#include <string>

#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/E133Header.h"
#include "plugins/e131/e131/E133Layer.h"
#include "plugins/e131/e131/RootLayer.h"
#include "plugins/e131/e131/UDPTransport.h"
#include "plugins/e131/e131/DMPE133Inflator.h"


using std::string;
using ola::network::IPV4Address;
using ola::network::SelectServer;
using ola::rdm::RDMRequest;
using ola::plugin::e131::E133Header;

typedef struct {
  bool help;
  ola::log_level log_level;
  unsigned int universe;
  string ip_address;
} options;


class E133Node {
  public:
    E133Node(const string &preferred_ip, unsigned int universe);

    bool Init();
    void Run() { m_ss.Run(); }

    void HandleManagementPacket(
        const IPV4Address &src_addr,
        const E133Header &e133_header,
        const string &request);
    void HandleRDMRequest(
        const IPV4Address &src_addr,
        const E133Header &e133_header,
        const string &request);

  private:
    const string m_preferred_ip;
    const unsigned int m_universe;
    ola::network::SelectServer m_ss;

    ola::plugin::e131::CID m_cid;
    ola::plugin::e131::UDPTransport m_transport;
    ola::plugin::e131::RootLayer m_root_layer;
    ola::plugin::e131::E133Layer m_e133_layer;
    ola::plugin::e131::DMPE133Inflator m_dmp_inflator;
};


E133Node::E133Node(const string &preferred_ip, unsigned int universe)
    : m_preferred_ip(preferred_ip),
      m_universe(universe),
      m_cid(ola::plugin::e131::CID::Generate()),
      m_transport(ola::plugin::e131::UDPTransport::ACN_PORT),
      m_root_layer(&m_transport, m_cid),
      m_e133_layer(&m_root_layer),
      m_dmp_inflator(&m_e133_layer) {
}


bool E133Node::Init() {
  ola::network::Interface interface;
  const ola::network::InterfacePicker *picker =
    ola::network::InterfacePicker::NewPicker();
  if (!picker->ChooseInterface(&interface, m_preferred_ip)) {
    OLA_INFO << "Failed to find an interface";
    delete picker;
    return false;
  }
  delete picker;
  OLA_INFO << "Using " << interface.ip_address;

  if (!m_transport.Init(interface)) {
    return false;
  }

  ola::network::UdpSocket *socket = m_transport.GetSocket();
  m_ss.AddSocket(socket);

  m_e133_layer.SetInflator(&m_dmp_inflator);

  // setup the callback to catch RDM packets
  m_dmp_inflator.SetRDMManagementhandler(
    ola::NewCallback(this, &E133Node::HandleManagementPacket));
  m_dmp_inflator.SetRDMHandler(
    m_universe,
    ola::NewCallback(this, &E133Node::HandleRDMRequest));

  return true;
}


/**
 * Called when we receive a management packet
 */
void E133Node::HandleManagementPacket(const IPV4Address &src_addr,
                                      const E133Header &e133_header,
                                      const string &request) {
  OLA_INFO << "got management packet from " << src_addr;
  (void) e133_header;
  (void) request;
}


/**
 * Called when we receive a rdm packet
 */
void E133Node::HandleRDMRequest(const IPV4Address &src_addr,
                                const E133Header &e133_header,
                                const string &request) {
  OLA_INFO << "Got RDM request on universe " << m_universe << ", from " <<
    src_addr;
  (void) e133_header;
  (void) request;
}


/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {"ip", required_argument, 0, 'i'},
      {"universe", required_argument, 0, 'u'},
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "hi:l:u:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
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
  "Run a very simple E133 Node.\n"
  "\n"
  "  -h, --help         Display this help message and exit.\n"
  "  -i, --ip           The IP address to listen on.\n"
  "  -l, --log-level <level>  Set the loggging level 0 .. 4.\n"
  "  -u, --universe <universe>  The universe to respond on (> 0).\n"
  << std::endl;
  exit(0);
}


/*
 * Startup a node
 */
int main(int argc, char *argv[]) {
  options opts;
  opts.log_level = ola::OLA_LOG_WARN;
  opts.universe = 1;
  opts.help = false;
  ParseOptions(argc, argv, &opts);

  if (opts.help)
    DisplayHelpAndExit(argv);
  ola::InitLogging(opts.log_level, ola::OLA_LOG_STDERR);

  E133Node node(opts.ip_address, opts.universe);
  if (node.Init())
    node.Run();
}
