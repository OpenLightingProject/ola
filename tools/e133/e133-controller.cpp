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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <errno.h>
#include <getopt.h>
#include <sysexits.h>

#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/io/SelectServer.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/Socket.h>
#include <ola/rdm/CommandPrinter.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/UID.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>


#include "plugins/e131/e131/ACNPort.h"
#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/E133Inflator.h"
#include "plugins/e131/e131/E133Sender.h"
#include "plugins/e131/e131/RDMInflator.h"
#include "plugins/e131/e131/RDMPDU.h"
#include "plugins/e131/e131/RootInflator.h"
#include "plugins/e131/e131/RootSender.h"
#include "plugins/e131/e131/UDPTransport.h"

#include "tools/e133/E133Endpoint.h"
#include "tools/e133/SlpThread.h"
#include "tools/e133/SlpUrlParser.h"

using ola::network::IPV4Address;
using ola::NewCallback;
using ola::network::UdpSocket;
using ola::plugin::e131::E133_PORT;
using ola::rdm::PidStoreHelper;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

typedef struct {
  uint16_t endpoint;
  bool help;
  ola::log_level log_level;
  bool rdm_set;
  string pid_file;
  string ip_address;
  string target_address;
  UID *uid;
  string pid;  // the pid to get
  vector<string> args;  // extra args
} options;


/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  int uid_set = 0;
  static struct option long_options[] = {
      {"endpoint", required_argument, 0, 'e'},
      {"help", no_argument, 0, 'h'},
      {"ip", required_argument, 0, 'i'},
      {"log-level", required_argument, 0, 'l'},
      {"pid-file", required_argument, 0, 'p'},
      {"set", no_argument, 0, 's'},
      {"target", required_argument, 0, 't'},
      {"uid", required_argument, &uid_set, 1},
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv,
                        "e:hi:l:p:st:",
                        long_options,
                        &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        if (uid_set)
          opts->uid = UID::FromString(optarg);
        break;
      case 'e':
        opts->endpoint = atoi(optarg);
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
      case 'p':
        opts->pid_file = optarg;
        break;
      case 's':
        opts->rdm_set = true;
        break;
      case 't':
        opts->target_address = optarg;
        break;
      case '?':
        break;
      default:
       break;
    }
  }

  int index = optind;
  for (; index < argc; index++)
    opts->args.push_back(argv[index]);
  return;
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char *argv[]) {
  cout << "Usage: " << argv[0] << " [options] <pid_name> [param_data]\n"
  "\n"
  "Search for a UID registered in SLP Send it a E1.33 Message.\n"
  "\n"
  "  -e, --endpoint <endpoint> The endpoint to use.\n"
  "  -h, --help                Display this help message and exit.\n"
  "  -t, --target <ip>         IP to send the message to, this overrides SLP\n"
  "  -i, --ip                  The IP address to listen on.\n"
  "  -l, --log-level <level>   Set the logging level 0 .. 4.\n"
  "  -p, --pid-file            The file to read PID definitiions from\n"
  "  -s, --set                 Perform a SET (default is GET)\n"
  "  --uid <uid>               The UID of the device to control.\n"
  << endl;
  exit(0);
}



/**
 * A very simple E1.33 Controller
 */
class SimpleE133Controller {
  public:
    struct SimpleE133ControllerOptions {
      IPV4Address controller_ip;
    };

    SimpleE133Controller(const SimpleE133ControllerOptions &options,
                         PidStoreHelper *pid_helper);
    ~SimpleE133Controller();

    bool Init();
    void PopulateResponderList();
    void AddUID(const UID &uid, const IPV4Address &ip);
    void Run();
    void Stop() { m_ss.Terminate(); }

    // very basic methods for sending RDM requests
    void SendGetRequest(const UID &dst_uid,
                        uint16_t endpoint,
                        uint16_t pid,
                        const uint8_t *data,
                        unsigned int data_length);
    void SendSetRequest(const UID &dst_uid,
                        uint16_t endpoint,
                        uint16_t pid,
                        const uint8_t *data,
                        unsigned int data_length);

  private:
    SimpleE133ControllerOptions m_options;
    ola::io::SelectServer m_ss;

    // The Controller's CID
    ola::plugin::e131::CID m_cid;

    // inflators
    ola::plugin::e131::RootInflator m_root_inflator;
    ola::plugin::e131::E133Inflator m_e133_inflator;
    ola::plugin::e131::RDMInflator m_rdm_inflator;

    // sockets & transports
    UdpSocket m_udp_socket;
    ola::plugin::e131::IncomingUDPTransport m_incoming_udp_transport;
    ola::plugin::e131::OutgoingUDPTransportImpl m_outgoing_udp_transport;

    // senders
    ola::plugin::e131::RootSender m_root_sender;
    ola::plugin::e131::E133Sender m_e133_sender;

    // hash_map of UIDs to IPs
    typedef std::map<UID, IPV4Address> uid_to_ip_map;
    uid_to_ip_map m_uid_to_ip;

    UID m_src_uid;
    SlpThread m_slp_thread;
    PidStoreHelper *m_pid_helper;
    ola::rdm::CommandPrinter m_command_printer;
    bool m_uid_list_updated;

    void DiscoveryCallback(bool status, const vector<string> &urls);
    bool SendRequest(const UID &uid, uint16_t endpoint, RDMRequest *request);
    void HandlePacket(
        const ola::plugin::e131::TransportHeader &transport_header,
        const ola::plugin::e131::E133Header &e133_header,
        const std::string &raw_response);
    void RequestCallback(ola::rdm::rdm_response_code rdm_code,
                         const RDMResponse *response,
                         const vector<std::string> &packets);
    void HandleNack(const RDMResponse *response);
};


/**
 * Setup our simple controller
 */
SimpleE133Controller::SimpleE133Controller(
    const SimpleE133ControllerOptions &options,
    PidStoreHelper *pid_helper)
    : m_options(options),
      m_cid(ola::plugin::e131::CID::Generate()),
      m_incoming_udp_transport(&m_udp_socket, &m_root_inflator),
      m_outgoing_udp_transport(&m_udp_socket),
      m_root_sender(m_cid),
      m_e133_sender(&m_root_sender),
      m_src_uid(OPEN_LIGHTING_ESTA_CODE, 0xabcdabcd),
      m_slp_thread(
        &m_ss,
        ola::NewCallback(this, &SimpleE133Controller::DiscoveryCallback)),
      m_pid_helper(pid_helper),
      m_command_printer(&cout, m_pid_helper),
      m_uid_list_updated(false) {
  m_root_inflator.AddInflator(&m_e133_inflator);
  m_e133_inflator.AddInflator(&m_rdm_inflator);
}


/**
 * Tear down
 */
SimpleE133Controller::~SimpleE133Controller() {
  m_slp_thread.Join();
  m_slp_thread.Cleanup();
}


/**
 * Start up the controller
 */
bool SimpleE133Controller::Init() {
  if (!m_udp_socket.Init())
    return false;

  if (!m_udp_socket.Bind(m_options.controller_ip, 0)) {
    OLA_INFO << "Failed to bind to UDP port";
    return false;
  }

  m_udp_socket.SetOnData(
      NewCallback(&m_incoming_udp_transport,
                  &ola::plugin::e131::IncomingUDPTransport::Receive));
  m_ss.AddReadDescriptor(&m_udp_socket);

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
  }
}


void SimpleE133Controller::AddUID(const UID &uid, const IPV4Address &ip) {
  OLA_INFO << "adding UID " << uid << " @ " << ip;
  m_uid_to_ip[uid] = ip;
}


/**
 * Run the controller and wait for the responses (or timeouts)
 */
void SimpleE133Controller::Run() {
  m_ss.Run();
}


/**
 * Send a GET request
 */
void SimpleE133Controller::SendGetRequest(const UID &dst_uid,
                                          uint16_t endpoint,
                                          uint16_t pid,
                                          const uint8_t *data,
                                          unsigned int length) {
  // send a second one
  ola::rdm::RDMGetRequest *command = new ola::rdm::RDMGetRequest(
      m_src_uid,
      dst_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      ola::rdm::ROOT_RDM_DEVICE,  // sub device
      pid,  // param id
      data,  // data
      length);  // data length

  if (!SendRequest(dst_uid, endpoint, command)) {
    OLA_FATAL << "Failed to send request";
    m_ss.Terminate();
  } else if (dst_uid.IsBroadcast()) {
    OLA_INFO << "Request broadcast";
    m_ss.Terminate();
  } else {
    OLA_INFO << "Request sent, waiting for response";
  }
}


/**
 * Send a SET request
 */
void SimpleE133Controller::SendSetRequest(const UID &dst_uid,
                                          uint16_t endpoint,
                                          uint16_t pid,
                                          const uint8_t *data,
                                          unsigned int data_length) {
  ola::rdm::RDMSetRequest *command = new ola::rdm::RDMSetRequest(
      m_src_uid,
      dst_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      ola::rdm::ROOT_RDM_DEVICE,  // sub device
      pid,  // param id
      data,  // data
      data_length);  // data length

  if (!SendRequest(dst_uid, endpoint, command)) {
    OLA_FATAL << "Failed to send request";
    m_ss.Terminate();
  } else {
    OLA_INFO << "Request sent";
  }
}


/**
 * Called when SLP discovery completes.
 */
void SimpleE133Controller::DiscoveryCallback(bool ok,
                                             const vector<string> &urls) {
  OLA_INFO << "in slp cb " << ok;
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
      AddUID(uid, ip);
    }
  }
  m_uid_list_updated = true;
  m_ss.Terminate();
}


/**
 * Send an RDM Request.
 * This packs the data into a ACN structure and sends it.
 */
bool SimpleE133Controller::SendRequest(const UID &uid,
                                       uint16_t endpoint,
                                       RDMRequest *request) {
  uid_to_ip_map::const_iterator iter = m_uid_to_ip.find(uid);
  if (iter == m_uid_to_ip.end()) {
    OLA_WARN << "UID " << uid << " not found";
    delete request;
    return false;
  }

  OLA_INFO << "Sending to " << iter->second << ":" << E133_PORT << "/" << uid
      << "/" << endpoint;

  const ola::plugin::e131::RDMPDU pdu(request);
  ola::plugin::e131::E133Header header(
      "E1.33 Controller",
      0,  // seq #
      endpoint,
      false);  // rx_ack

  ola::plugin::e131::OutgoingUDPTransport transport(&m_outgoing_udp_transport,
                                                    iter->second,
                                                    E133_PORT);
  bool result = m_e133_sender.SendRDM(header, &pdu, &transport);
  if (!result) {
    OLA_WARN << "Failed to send E1.33 request";
    return false;
  }

  // register a callback to catch the response
  m_rdm_inflator.SetRDMHandler(
      endpoint,
      NewCallback(this,
                  &SimpleE133Controller::HandlePacket));
  return true;
}



/**
 * Handle a RDM response addressed to this universe
 */
void SimpleE133Controller::HandlePacket(
    const ola::plugin::e131::TransportHeader &transport_header,
    const ola::plugin::e131::E133Header &e133_header,
    const std::string &raw_response) {
  // don't bother checking anything here
  (void) e133_header;

  // try to locate the pending request
  OLA_INFO << "Got data from " << transport_header.SourceIP();

  // attempt to unpack as a response
  ola::rdm::rdm_response_code response_code;
  const RDMResponse *response = RDMResponse::InflateFromData(
    reinterpret_cast<const uint8_t*>(raw_response.data()),
    raw_response.size(),
    &response_code);

  if (!response) {
    OLA_WARN << "Failed to unpack E1.33 RDM message, ignoring request.";
    return;
  }

  std::vector<std::string> raw_packets;
  raw_packets.push_back(raw_response);
  RequestCallback(response_code, response, raw_packets);
}

/**
 * Called when the RDM command completes
 */
void SimpleE133Controller::RequestCallback(
    ola::rdm::rdm_response_code rdm_code,
    const RDMResponse *response_ptr,
    const std::vector<std::string>&) {
  auto_ptr<const RDMResponse> response(response_ptr);
  OLA_INFO << "RDM callback executed with code: " <<
    ola::rdm::ResponseCodeToString(rdm_code);

  m_ss.Terminate();

  if (rdm_code != ola::rdm::RDM_COMPLETED_OK)
    return;

  switch (response->ResponseType()) {
    case ola::rdm::RDM_NACK_REASON:
      HandleNack(response.get());
      return;
    default:
      break;
  }

  const ola::rdm::PidDescriptor *pid_descriptor = m_pid_helper->GetDescriptor(
      response->ParamId(),
      response->SourceUID().ManufacturerId());
  const ola::messaging::Descriptor *descriptor = NULL;

  if (pid_descriptor) {
    switch (response->CommandClass()) {
      case ola::rdm::RDMCommand::GET_COMMAND_RESPONSE:
        descriptor = pid_descriptor->GetResponse();
        break;
      case ola::rdm::RDMCommand::SET_COMMAND_RESPONSE:
        descriptor = pid_descriptor->SetResponse();
        break;
      default:
        OLA_WARN << "Unknown command class " << response->CommandClass();
    }
  }

  auto_ptr<const ola::messaging::Message> message;
  if (descriptor)
    message.reset(m_pid_helper->DeserializeMessage(descriptor,
                                                   response->ParamData(),
                                                   response->ParamDataSize()));

  if (message.get())
    cout << m_pid_helper->PrettyPrintMessage(
        response->SourceUID().ManufacturerId(),
        response->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND_RESPONSE,
        response->ParamId(),
        message.get());
  else
    m_command_printer.DisplayResponse(response.get(), true);
}


/**
 * Handle a NACK response
 */
void SimpleE133Controller::HandleNack(const RDMResponse *response) {
  uint16_t param;
  if (response->ParamDataSize() != sizeof(param)) {
    OLA_WARN << "Request NACKed but has invalid PDL size of " <<
      response->ParamDataSize();
  } else {
    memcpy(&param, response->ParamData(), sizeof(param));
    param = ola::network::NetworkToHost(param);
    OLA_INFO << "Request NACKed: " <<
      ola::rdm::NackReasonToString(param);
  }
}


/*
 * Startup a node
 */
int main(int argc, char *argv[]) {
  options opts;
  opts.log_level = ola::OLA_LOG_WARN;
  opts.endpoint = ROOT_E133_ENDPOINT;
  opts.help = false;
  opts.pid_file = PID_DATA_FILE;
  opts.rdm_set = false;
  opts.uid = NULL;
  ParseOptions(argc, argv, &opts);
  PidStoreHelper pid_helper(opts.pid_file);

  if (opts.help)
    DisplayHelpAndExit(argv);

  ola::InitLogging(opts.log_level, ola::OLA_LOG_STDERR);

  // convert the controller's IP address, or use the wildcard if not specified
  IPV4Address controller_ip = IPV4Address::WildCard();
  if (!opts.ip_address.empty() &&
      !IPV4Address::FromString(opts.ip_address, &controller_ip))
    DisplayHelpAndExit(argv);

  // convert the node's IP address if specified
  IPV4Address target_ip;
  if (!opts.target_address.empty() &&
      !IPV4Address::FromString(opts.target_address, &target_ip))
    DisplayHelpAndExit(argv);

  // check the UID
  if (!opts.uid) {
    OLA_FATAL << "Invalid UID";
    exit(EX_USAGE);
  }
  UID dst_uid(*opts.uid);
  delete opts.uid;

  // Make sure we can load our PIDs
  if (!pid_helper.Init())
    exit(EX_OSFILE);

  if (opts.args.size() < 1) {
    DisplayHelpAndExit(argv);
    exit(EX_USAGE);
  }

  // get the pid descriptor
  const ola::rdm::PidDescriptor *pid_descriptor = pid_helper.GetDescriptor(
      opts.args[0],
      dst_uid.ManufacturerId());

  if (!pid_descriptor) {
    OLA_WARN << "Unknown PID: " << opts.args[0] << ".";
    OLA_WARN << "Use --pids to list the available PIDs.";
    exit(EX_USAGE);
  }

  const ola::messaging::Descriptor *descriptor = NULL;
  if (opts.rdm_set)
    descriptor = pid_descriptor->SetRequest();
  else
    descriptor = pid_descriptor->GetRequest();

  if (!descriptor) {
    OLA_WARN << (opts.rdm_set ? "SET" : "GET") << " command not supported for "
      << opts.args[0];
    exit(EX_USAGE);
  }

  // attempt to build the message
  vector<string> inputs(opts.args.size() - 1);
  vector<string>::iterator args_iter = opts.args.begin();
  copy(++args_iter, opts.args.end(), inputs.begin());
  auto_ptr<const ola::messaging::Message> message(pid_helper.BuildMessage(
      descriptor,
      inputs));

  if (!message.get()) {
    // print the schema here
    cout << pid_helper.SchemaAsString(descriptor);
    exit(EX_USAGE);
  }

  SimpleE133Controller::SimpleE133ControllerOptions controller_options;
  controller_options.controller_ip = controller_ip;

  SimpleE133Controller controller(controller_options, &pid_helper);
  if (!controller.Init()) {
    OLA_FATAL << "Failed to init controller";
    exit(EX_UNAVAILABLE);
  }

  if (target_ip.AsInt())
    // manually add the responder address
    controller.AddUID(dst_uid, target_ip);
  else
    // this blocks while the slp thread does it's thing
    controller.PopulateResponderList();

  // convert the message to binary form
  unsigned int param_data_length;
  const uint8_t *param_data = pid_helper.SerializeMessage(
      message.get(),
      &param_data_length);

  // send the message
  if (opts.rdm_set) {
    controller.SendSetRequest(dst_uid,
                              opts.endpoint,
                              pid_descriptor->Value(),
                              param_data,
                              param_data_length);
  } else {
    controller.SendGetRequest(dst_uid,
                              opts.endpoint,
                              pid_descriptor->Value(),
                              param_data,
                              param_data_length);
  }
  controller.Run();
}
