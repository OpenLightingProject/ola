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
 * e133-controller.cpp
 * Copyright (C) 2011 Simon Newton
 *
 * This locates all E1.33 devices using SLP and then searches for one that
 * matches the specified UID. If --target is used it skips the SLP skip.
 *
 * It then sends some RDM commands to the E1.33 node and waits for the
 * response.
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <errno.h>
#include <getopt.h>
#include <sysexits.h>

#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/SelectServer.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/UID.h>
#include <ola/StringUtils.h>

#include <iostream>
#include <string>
#include <vector>

#include "plugins/e131/e131/ACNPort.h"

#include "tools/e133/E133Node.h"
#include "tools/e133/E133UniverseController.h"
#include "tools/e133/SlpThread.h"
#include "tools/e133/SlpUrlParser.h"

using std::string;
using std::vector;
using ola::network::IPV4Address;
using ola::rdm::RDMRequest;
using ola::rdm::UID;

typedef struct {
  bool help;
  ola::log_level log_level;
  unsigned int universe;
  string ip_address;
  string target_address;
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
      {"target", required_argument, 0, 't'},
      {"universe", required_argument, 0, 'u'},
      {"uid", required_argument, &uid_set, 1},
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
        opts->target_address = optarg;
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
  "Search for a UID registered in SLP Send it a E1.33 Message.\n"
  "\n"
  "  -h, --help        Display this help message and exit.\n"
  "  -t, --target <ip> IP to send the message to, adding this overrides SLP\n"
  "  -i, --ip          The IP address to listen on.\n"
  "  -l, --log-level <level>   Set the loggging level 0 .. 4.\n"
  "  -u, --universe <universe> The universe to use (> 0).\n"
  "  --uid <uid>               The UID of the device to control.\n"
  << std::endl;
  exit(0);
}


class SimpleE133Controller {
  public:
    explicit SimpleE133Controller(const options &opts);
    ~SimpleE133Controller();

    bool Init();
    void PopulateResponderList();
    void AddUID(const UID &uid, const IPV4Address &ip);
    void Run();
    void Stop() { m_ss.Terminate(); }

    // very basic methods for sending RDM requests
    void SendGetRequest(const UID &dst_uid, ola::rdm::rdm_pid pid);
    void SendSetRequest(const UID &dst_uid,
                        ola::rdm::rdm_pid pid,
                        const uint8_t *data,
                        unsigned int data_length);

  private:
    ola::network::SelectServer m_ss;
    SlpThread m_slp_thread;
    E133Node m_e133_node;
    E133UniverseController m_controller;
    UID m_src_uid;
    unsigned int m_responses_to_go;
    unsigned int m_transaction_number;
    bool m_uid_list_updated;

    void DiscoveryCallback(bool status, const vector<string> &urls);
    void RequestCallback(ola::rdm::rdm_response_code rdm_code,
                         const ola::rdm::RDMResponse *response,
                         const std::vector<std::string> &packets);
};



SimpleE133Controller::SimpleE133Controller(const options &opts)
    : m_slp_thread(
        &m_ss,
        ola::NewCallback(this, &SimpleE133Controller::DiscoveryCallback)),
      m_e133_node(&m_ss, opts.ip_address, ola::plugin::e131::ACN_PORT + 1),
      m_controller(opts.universe),
      m_src_uid(OPEN_LIGHTING_ESTA_CODE, 0xabcdabcd),
      m_responses_to_go(0),
      m_transaction_number(0),
      m_uid_list_updated(false) {
  m_e133_node.RegisterComponent(&m_controller);
}


SimpleE133Controller::~SimpleE133Controller() {
  m_e133_node.UnRegisterComponent(&m_controller);
  m_slp_thread.Join();
  m_slp_thread.Cleanup();
}


bool SimpleE133Controller::Init() {
  if (!m_e133_node.Init())
    return false;

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
void SimpleE133Controller::PopulateResponderList() {
  if (!m_uid_list_updated) {
    m_slp_thread.Discover();
    // if we don't have a up to date list wait for slp to return
    m_ss.Run();
    m_ss.Restart();
  }
}


void SimpleE133Controller::AddUID(const UID &uid, const IPV4Address &ip) {
  m_controller.AddUID(uid, ip);
}


/**
 * Run the controller and wait for the responses (or timeouts)
 */
void SimpleE133Controller::Run() {
  if (m_responses_to_go)
    m_ss.Run();
}



void SimpleE133Controller::DiscoveryCallback(bool ok,
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
      OLA_INFO << "Adding " << uid << "@" << ip;
      m_controller.AddUID(uid, ip);
    }
  }
  m_uid_list_updated = true;
  m_ss.Terminate();
}


/**
 * Called when the RDM command completes
 */
void SimpleE133Controller::RequestCallback(
    ola::rdm::rdm_response_code rdm_code,
    const ola::rdm::RDMResponse *response,
    const std::vector<std::string> &packets) {
  OLA_INFO << "Callback executed with code: " <<
    ola::rdm::ResponseCodeToString(rdm_code);

  if (rdm_code == ola::rdm::RDM_COMPLETED_OK) {
    OLA_INFO << response->SourceUID() << " -> " << response->DestinationUID()
      << ", TN: " << (int) response->TransactionNumber() << ", Msg Count: " <<
      (int) response->MessageCount() << ", sub dev: " << response->SubDevice()
      << ", param 0x" << std::hex << response->ParamId() << ", data len: " <<
      std::dec << (int) response->ParamDataSize();
  }
  delete response;

  if (!--m_responses_to_go)
    m_ss.Terminate();
  (void) packets;
}


void SimpleE133Controller::SendGetRequest(const UID &dst_uid,
                                          ola::rdm::rdm_pid pid) {
  // send a second one
  ola::rdm::RDMGetRequest *command = new ola::rdm::RDMGetRequest(
      m_src_uid,
      dst_uid,
      m_transaction_number++,  // transaction #
      1,  // port id
      0,  // message count
      ola::rdm::ROOT_RDM_DEVICE,  // sub device
      pid,  // param id
      NULL,  // data
      0);  // data length

  m_controller.SendRDMRequest(
      command,
      ola::NewSingleCallback(this, &SimpleE133Controller::RequestCallback));
  m_responses_to_go++;
}



void SimpleE133Controller::SendSetRequest(const UID &dst_uid,
                                          ola::rdm::rdm_pid pid,
                                          const uint8_t *data,
                                          unsigned int data_length) {
  ola::rdm::RDMSetRequest *command = new ola::rdm::RDMSetRequest(
      m_src_uid,
      dst_uid,
      m_transaction_number++,  // transaction #
      1,  // port id
      0,  // message count
      ola::rdm::ROOT_RDM_DEVICE,  // sub device
      pid,  // param id
      data,  // data
      data_length);  // data length

  m_controller.SendRDMRequest(
      command,
      ola::NewSingleCallback(this, &SimpleE133Controller::RequestCallback));
  m_responses_to_go++;
}


/*
 * Startup a node
 */
int main(int argc, char *argv[]) {
  options opts;
  opts.log_level = ola::OLA_LOG_WARN;
  opts.universe = 1;
  opts.help = false;
  opts.uid = NULL;
  ParseOptions(argc, argv, &opts);

  if (opts.help)
    DisplayHelpAndExit(argv);

  ola::InitLogging(opts.log_level, ola::OLA_LOG_STDERR);

  IPV4Address target_ip;
  if (!opts.target_address.empty() &&
      !IPV4Address::FromString(opts.target_address, &target_ip))
    DisplayHelpAndExit(argv);

  if (!opts.uid) {
    OLA_FATAL << "Invalid UID";
    exit(EX_USAGE);
  }
  UID dst_uid(*opts.uid);
  delete opts.uid;

  SimpleE133Controller controller(opts);
  if (!controller.Init())
    exit(EX_UNAVAILABLE);

  if (opts.target_address.empty())
    // this blocks while the slp thread does it's thing
    controller.PopulateResponderList();
  else
    // manually add the responder address
    controller.AddUID(dst_uid, target_ip);

  // send some rdm get messages
  controller.SendGetRequest(dst_uid, ola::rdm::PID_DEVICE_INFO);
  controller.SendGetRequest(dst_uid, ola::rdm::PID_SOFTWARE_VERSION_LABEL);

  // now send a set message
  uint16_t start_address = 10;
  start_address = ola::network::HostToNetwork(start_address);
  controller.SendSetRequest(dst_uid,
                            ola::rdm::PID_DMX_START_ADDRESS,
                            reinterpret_cast<uint8_t*>(&start_address),
                            sizeof(start_address));

  controller.Run();
}
