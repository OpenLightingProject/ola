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
#include <ola/StringUtils.h>
#include <ola/messaging/Descriptor.h>
#include <ola/messaging/Message.h>
#include <ola/messaging/MessagePrinter.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/SelectServer.h>
#include <ola/rdm/MessageDeserializer.h>
#include <ola/rdm/MessageSerializer.h>
#include <ola/rdm/PidStore.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/StringMessageBuilder.h>
#include <ola/rdm/UID.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "plugins/e131/e131/ACNPort.h"

#include "tools/e133/E133Node.h"
#include "tools/e133/E133UniverseController.h"
#include "tools/e133/SlpThread.h"
#include "tools/e133/SlpUrlParser.h"

using ola::network::IPV4Address;
using ola::rdm::RDMRequest;
using ola::rdm::UID;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

typedef struct {
  bool help;
  bool list_pids;
  bool rdm_set;
  ola::log_level log_level;
  unsigned int universe;
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
      {"help", no_argument, 0, 'h'},
      {"ip", required_argument, 0, 'i'},
      {"log-level", required_argument, 0, 'l'},
      {"pids", no_argument, 0, 'p'},
      {"set", no_argument, 0, 's'},
      {"target", required_argument, 0, 't'},
      {"universe", required_argument, 0, 'u'},
      {"uid", required_argument, &uid_set, 1},
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "hi:l:pst:u:", long_options, &option_index);

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
      case 'p':
        opts->list_pids = true;
        break;
      case 's':
        opts->rdm_set = true;
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
  "  -h, --help                Display this help message and exit.\n"
  "  -t, --target <ip>         IP to send the message to, this overrides SLP\n"
  "  -i, --ip                  The IP address to listen on.\n"
  "  -l, --log-level <level>   Set the loggging level 0 .. 4.\n"
  "  -p, --pids                List the available pids for this device\n"
  "  -s, --set                 Perform a SET (default is GET)\n"
  "  -u, --universe <universe> The universe to use (> 0).\n"
  "  --uid <uid>               The UID of the device to control.\n"
  << endl;
  exit(0);
}


/**
 * A class which manages the RDM PIDs
 */
class RDMPidHelper {
  public:
    explicit RDMPidHelper(const string &pid_file);
    ~RDMPidHelper();

    bool Init();

    const ola::rdm::PidDescriptor *GetDescriptor(uint16_t manufacturer,
                                                 const string &pid_name) const;
    const ola::rdm::PidDescriptor *GetDescriptor(uint16_t manufacturer,
                                                 uint16_t param_id) const;

    const ola::messaging::Message *BuildMessage(
        const ola::messaging::Descriptor *descriptor,
        const vector<string> &inputs);

    const uint8_t *SerializeMessage(const ola::messaging::Message *message,
                                    unsigned int *data_length);

    const ola::messaging::Message *DeserializeMessage(
        const ola::messaging::Descriptor *descriptor,
        const uint8_t *data,
        unsigned int data_length);

    const string MessageToString(const ola::messaging::Message *message);

    void SupportedPids(uint16_t manufacturer_id,
                       vector<string> *pid_names) const;

  private:
    const string m_pid_file;
    const ola::rdm::RootPidStore *m_root_store;
    ola::rdm::StringMessageBuilder m_string_builder;
    ola::rdm::MessageSerializer m_serializer;
    ola::rdm::MessageDeserializer m_deserializer;
    ola::messaging::MessagePrinter m_message_printer;
};


/**
 * Set up a new RDMPidHelper object
 */
RDMPidHelper::RDMPidHelper(const string &pid_file)
    : m_pid_file(pid_file),
      m_root_store(NULL) {
}


/**
 * Clean up
 */
RDMPidHelper::~RDMPidHelper() {
  if (m_root_store)
    delete m_root_store;
}


/**
 * Init the RDMPidHelper, this loads the pid store
 */
bool RDMPidHelper::Init() {
  if (m_root_store) {
    OLA_WARN << "Root Pid Store already loaded: " << m_pid_file;
    return false;
  }

  m_root_store = ola::rdm::RootPidStore::LoadFromFile(m_pid_file);
  return m_root_store;
}


const ola::rdm::PidDescriptor *RDMPidHelper::GetDescriptor(
    uint16_t manufacturer_id,
    const string &pid_name) const {
  if (!m_root_store)
    return NULL;

  string canonical_pid_name = pid_name;
  ola::ToUpper(&canonical_pid_name);

  const ola::rdm::PidStore *store = m_root_store->EstaStore();
  if (store) {
    const ola::rdm::PidDescriptor *descriptor =
      store->LookupPID(canonical_pid_name);
    if (descriptor)
      return descriptor;
  }

  // now try the specific manufacturer store
  store = m_root_store->ManufacturerStore(manufacturer_id);
  if (store) {
    const ola::rdm::PidDescriptor *descriptor =
      store->LookupPID(canonical_pid_name);
    if (descriptor)
      return descriptor;
  }
  return NULL;
}


/**
 * Get a RDM descriptor by value
 */
const ola::rdm::PidDescriptor *RDMPidHelper::GetDescriptor(
    uint16_t manufacturer_id,
    uint16_t param_id) const {
  if (!m_root_store)
    return NULL;

  const ola::rdm::PidStore *store = m_root_store->EstaStore();
  if (store) {
    const ola::rdm::PidDescriptor *descriptor =
      store->LookupPID(param_id);
    if (descriptor)
      return descriptor;
  }

  // now try the specific manufacturer store
  store = m_root_store->ManufacturerStore(manufacturer_id);
  if (store) {
    const ola::rdm::PidDescriptor *descriptor =
      store->LookupPID(param_id);
    if (descriptor)
      return descriptor;
  }
  return NULL;
}


/**
 * Build a Message object from a series of input strings
 */
const ola::messaging::Message *RDMPidHelper::BuildMessage(
    const ola::messaging::Descriptor *descriptor,
    const vector<string> &inputs) {

  const ola::messaging::Message *message = m_string_builder.GetMessage(
      inputs,
      descriptor);
  if (!message)
    OLA_WARN << "Error building message: " << m_string_builder.GetError();
  return message;
}


/**
 * Serialize a message to binary format
 */
const uint8_t *RDMPidHelper::SerializeMessage(
    const ola::messaging::Message *message,
    unsigned int *data_length) {
  return m_serializer.SerializeMessage(message, data_length);
}



/**
 * DeSerialize a message
 */
const ola::messaging::Message *RDMPidHelper::DeserializeMessage(
    const ola::messaging::Descriptor *descriptor,
    const uint8_t *data,
    unsigned int data_length) {
  return m_deserializer.InflateMessage(descriptor, data, data_length);
}


/**
 * Convert a message to a string
 */
const string RDMPidHelper::MessageToString(
    const ola::messaging::Message *message) {
  return m_message_printer.AsString(message);
}


/**
 * Return the list of pids supported including manufacturer pids.
 */
void RDMPidHelper::SupportedPids(uint16_t manufacturer_id,
                                 vector<string> *pid_names) const {
  if (!m_root_store)
    return;

  vector<const ola::rdm::PidDescriptor*> descriptors;
  const ola::rdm::PidStore *store = m_root_store->EstaStore();
  if (store)
    store->AllPids(&descriptors);

  store = m_root_store->ManufacturerStore(manufacturer_id);
  if (store)
    store->AllPids(&descriptors);

  vector<const ola::rdm::PidDescriptor*>::const_iterator iter;
  for (iter = descriptors.begin(); iter != descriptors.end(); ++iter) {
    string name = (*iter)->Name();
    ola::ToLower(&name);
    pid_names->push_back(name);
  }
}


/**
 * A very simple E1.33 Controller
 */
class SimpleE133Controller {
  public:
    explicit SimpleE133Controller(const options &opts,
                                  RDMPidHelper *pid_helper);
    ~SimpleE133Controller();

    bool Init();
    void PopulateResponderList();
    void AddUID(const UID &uid, const IPV4Address &ip);
    void Run();
    void Stop() { m_ss.Terminate(); }

    // very basic methods for sending RDM requests
    void SendGetRequest(const UID &dst_uid,
                        uint16_t pid,
                        const uint8_t *data,
                        unsigned int data_length);
    void SendSetRequest(const UID &dst_uid,
                        uint16_t pid,
                        const uint8_t *data,
                        unsigned int data_length);

  private:
    RDMPidHelper *m_pid_helper;
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



SimpleE133Controller::SimpleE133Controller(
    const options &opts,
    RDMPidHelper *pid_helper)
    : m_pid_helper(pid_helper),
      m_slp_thread(
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
  cout << "RDM callback executed with code: " <<
    ola::rdm::ResponseCodeToString(rdm_code) << endl;

  if (!--m_responses_to_go)
    m_ss.Terminate();

  if (rdm_code == ola::rdm::RDM_COMPLETED_OK) {
    const ola::rdm::PidDescriptor *pid_descriptor = m_pid_helper->GetDescriptor(
        response->SourceUID().ManufacturerId(),
        response->ParamId());
    const ola::messaging::Descriptor *descriptor = NULL;
    const ola::messaging::Message *message = NULL;

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
    if (descriptor) {
      message = m_pid_helper->DeserializeMessage(descriptor,
                                                 response->ParamData(),
                                                 response->ParamDataSize());
    }


    if (message) {
      cout << response->SourceUID() << " -> " << response->DestinationUID() <<
        endl;
      cout << m_pid_helper->MessageToString(message);
    } else {
      cout << response->SourceUID() << " -> " << response->DestinationUID()
        << ", TN: " << static_cast<int>(response->TransactionNumber()) <<
        ", Msg Count: " << static_cast<int>(response->MessageCount()) <<
        ", sub dev: " << response->SubDevice() << ", param 0x" << std::hex <<
        response->ParamId() << ", data len: " <<
        std::dec << static_cast<int>(response->ParamDataSize()) << endl;
    }

    if (message)
      delete message;
  }
  delete response;

  (void) packets;
}


void SimpleE133Controller::SendGetRequest(const UID &dst_uid,
                                          uint16_t pid,
                                          const uint8_t *data,
                                          unsigned int length) {
  // send a second one
  ola::rdm::RDMGetRequest *command = new ola::rdm::RDMGetRequest(
      m_src_uid,
      dst_uid,
      m_transaction_number++,  // transaction #
      1,  // port id
      0,  // message count
      ola::rdm::ROOT_RDM_DEVICE,  // sub device
      pid,  // param id
      data,  // data
      length);  // data length

  m_controller.SendRDMRequest(
      command,
      ola::NewSingleCallback(this, &SimpleE133Controller::RequestCallback));
  m_responses_to_go++;
}



void SimpleE133Controller::SendSetRequest(const UID &dst_uid,
                                          uint16_t pid,
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


/**
 * List the available pids for a device
 */
void ListPids(uint16_t manufacturer_id, const RDMPidHelper &pid_helper) {
  vector<string> pid_names;
  pid_helper.SupportedPids(manufacturer_id, &pid_names);
  sort(pid_names.begin(), pid_names.end());

  vector<string>::const_iterator iter = pid_names.begin();
  for (; iter != pid_names.end(); ++iter) {
    std::cout << *iter << std::endl;
  }
}


/*
 * Startup a node
 */
int main(int argc, char *argv[]) {
  options opts;
  opts.log_level = ola::OLA_LOG_WARN;
  opts.universe = 1;
  opts.help = false;
  opts.list_pids = false;
  opts.rdm_set = false;
  opts.uid = NULL;
  ParseOptions(argc, argv, &opts);
  RDMPidHelper pid_helper(PID_DATA_FILE);

  if (opts.help)
    DisplayHelpAndExit(argv);

  ola::InitLogging(opts.log_level, ola::OLA_LOG_STDERR);

  IPV4Address target_ip;
  if (!opts.list_pids &&
      !opts.target_address.empty() &&
      !IPV4Address::FromString(opts.target_address, &target_ip))
    DisplayHelpAndExit(argv);

  if (!opts.uid) {
    OLA_FATAL << "Invalid UID";
    exit(EX_USAGE);
  }
  UID dst_uid(*opts.uid);
  delete opts.uid;

  if (!pid_helper.Init())
    exit(EX_OSFILE);

  if (opts.list_pids) {
    ListPids(dst_uid.ManufacturerId(), pid_helper);
    exit(EX_OK);
  }

  if (opts.args.size() < 1) {
    DisplayHelpAndExit(argv);
    exit(EX_USAGE);
  }

  // get the pid descriptor
  const ola::rdm::PidDescriptor *pid_descriptor = pid_helper.GetDescriptor(
      dst_uid.ManufacturerId(),
      opts.args[0]);

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
  vector<string> inputs;
  vector<string>::iterator args_iter = opts.args.begin();
  inputs.reserve(opts.args.size() - 1);
  copy(++args_iter, opts.args.end(), inputs.begin());
  auto_ptr<const ola::messaging::Message> message(pid_helper.BuildMessage(
      descriptor,
      inputs));

  if (!message.get()) {
    // print the schema here
    exit(EX_USAGE);
  }

  SimpleE133Controller controller(opts, &pid_helper);
  if (!controller.Init())
    exit(EX_UNAVAILABLE);

  if (opts.target_address.empty())
    // this blocks while the slp thread does it's thing
    controller.PopulateResponderList();
  else
    // manually add the responder address
    controller.AddUID(dst_uid, target_ip);

  // convert the message to binary form
  unsigned int param_data_length;
  const uint8_t *param_data = pid_helper.SerializeMessage(
      message.get(),
      &param_data_length);

  // send the message
  if (opts.rdm_set) {
    controller.SendSetRequest(dst_uid,
                              pid_descriptor->Value(),
                              param_data,
                              param_data_length);
  } else {
    controller.SendGetRequest(dst_uid,
                              pid_descriptor->Value(),
                              param_data,
                              param_data_length);
  }
  controller.Run();
}
