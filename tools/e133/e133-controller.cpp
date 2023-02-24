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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * e133-controller.cpp
 * Copyright (C) 2011 Simon Newton
 *
 * This connects to the device specified in \--target.
 *
 * It then sends some RDM commands to the E1.33 node and waits for the
 * response.
 */

#include <ola/Callback.h>
#include <ola/Constants.h>
#include <ola/Logging.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/acn/ACNPort.h>
#include <ola/acn/ACNVectors.h>
#include <ola/acn/CID.h>
#include <ola/e133/E133StatusHelper.h>
#include <ola/e133/E133Receiver.h>
#include <ola/e133/MessageBuilder.h>
#include <ola/io/SelectServer.h>
#include <ola/io/IOStack.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/Socket.h>
#include <ola/rdm/CommandPrinter.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMCommandSerializer.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/UID.h>
#include <ola/stl/STLUtils.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "libs/acn/RDMPDU.h"

DEFINE_s_uint16(endpoint, e, 0, "The endpoint to use");
DEFINE_s_string(target, t, "", "List of IPs to connect to");
DEFINE_string(listen_ip, "", "The IP address to listen on");
DEFINE_s_string(pid_location, p, "",
                "The directory to read PID definitions from");
DEFINE_s_default_bool(set, s, false, "Perform a SET (default is GET)");
DEFINE_default_bool(list_pids, false, "Display a list of pids");
DEFINE_s_string(uid, u, "", "The UID of the device to control.");

using ola::NewCallback;
using ola::io::IOStack;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::UDPSocket;
using ola::acn::E133_PORT;
using ola::acn::RDMPDU;
using ola::rdm::PidStoreHelper;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

/*
 * Dump the list of known pids
 */
void DisplayPIDsAndExit(uint16_t manufacturer_id,
                        const PidStoreHelper &pid_helper) {
  vector<string> pid_names;
  pid_helper.SupportedPids(manufacturer_id, &pid_names);
  sort(pid_names.begin(), pid_names.end());

  vector<string>::const_iterator iter = pid_names.begin();
  for (; iter != pid_names.end(); ++iter) {
    cout << *iter << endl;
  }
  exit(ola::EXIT_OK);
}

/**
 * A very simple E1.33 Controller
 */
class SimpleE133Controller {
 public:
    struct Options {
      IPV4Address controller_ip;

      explicit Options(const IPV4Address &ip)
          : controller_ip(ip) {
      }
    };

    SimpleE133Controller(const Options &options,
                         PidStoreHelper *pid_helper);
    ~SimpleE133Controller();

    bool Init();
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
    const IPV4Address m_controller_ip;
    ola::io::SelectServer m_ss;

    ola::e133::MessageBuilder m_message_builder;

    // sockets & transports
    UDPSocket m_udp_socket;
    ola::e133::E133Receiver m_e133_receiver;

    // hash_map of UIDs to IPs
    typedef std::map<UID, IPV4Address> uid_to_ip_map;
    uid_to_ip_map m_uid_to_ip;

    UID m_src_uid;
    PidStoreHelper *m_pid_helper;
    ola::rdm::CommandPrinter m_command_printer;

    bool SendRequest(const UID &uid, uint16_t endpoint, RDMRequest *request);
    void HandlePacket(const ola::e133::E133RDMMessage &rdm_message);
    void HandleNack(const RDMResponse *response);

    void HandleStatusMessage(
        const ola::e133::E133StatusMessage &status_message);
};


/**
 * Setup our simple controller
 */
SimpleE133Controller::SimpleE133Controller(
    const Options &options,
    PidStoreHelper *pid_helper)
    : m_controller_ip(options.controller_ip),
      m_message_builder(ola::acn::CID::Generate(), "E1.33 Controller"),
      m_e133_receiver(
          &m_udp_socket,
          NewCallback(this, &SimpleE133Controller::HandleStatusMessage),
          NewCallback(this, &SimpleE133Controller::HandlePacket)),
      m_src_uid(ola::OPEN_LIGHTING_ESTA_CODE, 0xabcdabcd),
      m_pid_helper(pid_helper),
      m_command_printer(&cout, m_pid_helper) {
}


/**
 * Tear down
 */
SimpleE133Controller::~SimpleE133Controller() {
  // This used to stop the SLP thread.
}


/**
 * Start up the controller
 */
bool SimpleE133Controller::Init() {
  if (!m_udp_socket.Init())
    return false;

  if (!m_udp_socket.Bind(IPV4SocketAddress(m_controller_ip, 0))) {
    OLA_INFO << "Failed to bind to UDP port";
    return false;
  }

  m_ss.AddReadDescriptor(&m_udp_socket);

  // Previously this started the SLP thread.
  return true;
}

void SimpleE133Controller::AddUID(const UID &uid, const IPV4Address &ip) {
  OLA_INFO << "Adding UID " << uid << " @ " << ip;
  ola::STLReplace(&m_uid_to_ip, uid, ip);
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
 * Send an RDM Request.
 * This packs the data into a ACN structure and sends it.
 */
bool SimpleE133Controller::SendRequest(const UID &uid,
                                       uint16_t endpoint,
                                       RDMRequest *raw_request) {
  auto_ptr<RDMRequest> request(raw_request);

  IPV4Address *target_address = ola::STLFind(&m_uid_to_ip, uid);
  if (!target_address) {
    OLA_WARN << "UID " << uid << " not found";
    return false;
  }

  IPV4SocketAddress target(*target_address, E133_PORT);
  OLA_INFO << "Sending to " << target << "/" << uid << "/" << endpoint;

  // Build the E1.33 packet.
  IOStack packet(m_message_builder.pool());
  RDMCommandSerializer::Write(*request, &packet);
  RDMPDU::PrependPDU(&packet);
  m_message_builder.BuildUDPRootE133(
      &packet, ola::acn::VECTOR_FRAMING_RDMNET, 0, endpoint);

  // Send the packet
  m_udp_socket.SendTo(&packet, target);
  if (!packet.Empty()) {
    return false;
  }

  return true;
}



/**
 * Handle a RDM message.
 */
void SimpleE133Controller::HandlePacket(
    const ola::e133::E133RDMMessage &rdm_message) {
  OLA_INFO << "RDM callback executed with code: " <<
    ola::rdm::StatusCodeToString(rdm_message.status_code);

  m_ss.Terminate();

  if (rdm_message.status_code != ola::rdm::RDM_COMPLETED_OK)
    return;

  switch (rdm_message.response->ResponseType()) {
    case ola::rdm::RDM_NACK_REASON:
      HandleNack(rdm_message.response);
      return;
    default:
      break;
  }

  const RDMResponse *response = rdm_message.response;

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
    m_command_printer.DisplayResponse(response, true);
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


void SimpleE133Controller::HandleStatusMessage(
    const ola::e133::E133StatusMessage &status_message) {
  // TODO(simon): match src IP, sequence # etc. here.
  OLA_INFO << "Got status code from " << status_message.ip;

  ola::e133::E133StatusCode e133_status_code;
  if (!ola::e133::IntToStatusCode(status_message.status_code,
                                  &e133_status_code)) {
    OLA_INFO << "Unknown E1.33 Status code " << status_message.status_code
             << " : " << status_message.status_message;
  } else {
    OLA_INFO << "Device returned code " << status_message.status_code << " : "
             << ola::e133::StatusMessageIdToString(e133_status_code)
             << " : " << status_message.status_message;
  }
  Stop();
}


/*
 * Startup a node
 */
int main(int argc, char *argv[]) {
  ola::AppInit(&argc, argv, "[options]", "E1.33 Controller.");

  PidStoreHelper pid_helper(FLAGS_pid_location.str());

  // convert the controller's IP address, or use the wildcard if not specified
  IPV4Address controller_ip = IPV4Address::WildCard();
  if (!FLAGS_listen_ip.str().empty() &&
      !IPV4Address::FromString(FLAGS_listen_ip, &controller_ip)) {
    ola::DisplayUsage();
    exit(ola::EXIT_USAGE);
  }

  // convert the node's IP address if specified
  IPV4Address target_ip;
  if (!IPV4Address::FromString(FLAGS_target, &target_ip)) {
    ola::DisplayUsage();
    exit(ola::EXIT_USAGE);
  }

  auto_ptr<UID> uid(UID::FromString(FLAGS_uid));

  // Make sure we can load our PIDs
  if (!pid_helper.Init())
    exit(ola::EXIT_OSFILE);

  if (FLAGS_list_pids)
    DisplayPIDsAndExit(uid.get() ? uid->ManufacturerId() : 0, pid_helper);

  // check the UID
  if (!uid.get()) {
    OLA_FATAL << "Invalid or missing UID, try xxxx:yyyyyyyy";
    ola::DisplayUsage();
    exit(ola::EXIT_USAGE);
  }

  if (argc < 2) {
    ola::DisplayUsage();
    exit(ola::EXIT_USAGE);
  }

  // get the pid descriptor
  const ola::rdm::PidDescriptor *pid_descriptor = pid_helper.GetDescriptor(
      argv[1], uid->ManufacturerId());

  if (!pid_descriptor) {
    OLA_WARN << "Unknown PID: " << argv[1] << ".";
    OLA_WARN << "Use --list-pids to list the available PIDs.";
    exit(ola::EXIT_USAGE);
  }

  const ola::messaging::Descriptor *descriptor = NULL;
  if (FLAGS_set)
    descriptor = pid_descriptor->SetRequest();
  else
    descriptor = pid_descriptor->GetRequest();

  if (!descriptor) {
    OLA_WARN << (FLAGS_set ? "SET" : "GET") << " command not supported for "
             << argv[1];
    exit(ola::EXIT_USAGE);
  }

  // attempt to build the message
  vector<string> inputs;
  for (int i = 2; i < argc; i++)
    inputs.push_back(argv[i]);
  auto_ptr<const ola::messaging::Message> message(
      pid_helper.BuildMessage(descriptor, inputs));

  if (!message.get()) {
    cout << pid_helper.SchemaAsString(descriptor);
    exit(ola::EXIT_USAGE);
  }

  SimpleE133Controller controller(
      SimpleE133Controller::Options(controller_ip),
      &pid_helper);

  if (!controller.Init()) {
    OLA_FATAL << "Failed to init controller";
    exit(ola::EXIT_UNAVAILABLE);
  }

  // manually add the responder address
  controller.AddUID(*uid, target_ip);

  // convert the message to binary form
  unsigned int param_data_length;
  const uint8_t *param_data = pid_helper.SerializeMessage(
      message.get(), &param_data_length);

  // send the message
  if (FLAGS_set) {
    controller.SendSetRequest(*uid,
                              FLAGS_endpoint,
                              pid_descriptor->Value(),
                              param_data,
                              param_data_length);
  } else {
    controller.SendGetRequest(*uid,
                              FLAGS_endpoint,
                              pid_descriptor->Value(),
                              param_data,
                              param_data_length);
  }
  controller.Run();
}
