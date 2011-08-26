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
 *  ola-rdm.cpp
 *  The command line tool for controlling RDM devices
 *  Copyright (C) 2010 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <sysexits.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/OlaCallbackClient.h>
#include <ola/OlaClientWrapper.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMAPIImplInterface.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/UID.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

using ola::rdm::PidStoreHelper;
using ola::rdm::UID;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

typedef struct {
  bool set_mode;
  bool help;       // show the help
  bool list_pids;  // show the pid list
  int universe;         // universe id
  UID *uid;         // uid
  uint16_t sub_device;  // the sub device
  string pid;      // pid to get/set
  vector<string> args;  // extra args
  string cmd;  // argv[0]
} options;



/*
 * parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  opts->cmd = argv[0];
  opts->set_mode = false;
  opts->list_pids = false;
  opts->help = false;
  opts->universe = 1;
  opts->uid = NULL;
  opts->sub_device = 0;

  std::vector<string> tokens;
  ola::StringSplit(argv[0], tokens, "/");

  if (string(tokens[tokens.size() - 1]) == "ola_rdm_set")
    opts->set_mode = true;

  int uid_set = 0;
  static struct option long_options[] = {
      {"sub_device", required_argument, 0, 'd'},
      {"help", no_argument, 0, 'h'},
      {"list_pids", no_argument, 0, 'l'},
      {"universe", required_argument, 0, 'u'},
      {"uid", required_argument, &uid_set, 1},
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "d:lu:hf", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        if (uid_set)
          opts->uid = UID::FromString(optarg);
        break;
      case 'd':
        opts->sub_device = atoi(optarg);
        break;
      case 'h':
        opts->help = true;
        break;
      case 'l':
        opts->list_pids = true;
        break;
      case 'u':
        opts->universe = atoi(optarg);
        break;
      default:
        break;
    }
  }

  int index = optind;
  for (; index < argc; index++)
    opts->args.push_back(argv[index]);
}


/*
 * Display the help for get_pid
 */
void DisplayGetPidHelp(const options &opts) {
  cout << "usage: " << opts.cmd <<
  " --universe <universe> --uid <uid> <pid> <value>\n"
  "\n"
  "Get the value of a pid for a device.\n"
  "Use '" << opts.cmd << " --list_pids' to get a list of pids.\n"
  "\n"
  "  -d, --sub_device <device> target a particular sub device (default is 0)\n"
  "  -h, --help                display this help message and exit.\n"
  "  -l, --list_pids           display a list of pids\n"
  "  -u, --universe <universe> universe number.\n"
  "  --uid <uid>               the UID of the device to control.\n"
  << endl;
}


/*
 * Display the help for set_pid
 */
void DisplaySetPidHelp(const options &opts) {
  cout << "usage: " << opts.cmd <<
  " --universe <universe> --uid <uid> <pid> <value>\n"
  "\n"
  "Set the value of a pid for a device.\n"
  "Use '" << opts.cmd << " --list_pids' to get a list of pids.\n"
  "\n"
  "  -d, --sub_device <device> target a particular sub device (default is 0)\n"
  "  -h, --help                display this help message and exit.\n"
  "  -l, --list_pids           display a list of pids\n"
  "  -u, --universe <universe> universe number.\n"
  "  --uid <uid>               the UID of the device to control.\n"
  << endl;
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(const options &opts) {
  if (opts.set_mode) {
    DisplaySetPidHelp(opts);
  } else {
    DisplayGetPidHelp(opts);
  }
  exit(EX_USAGE);
}


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
  exit(EX_OK);
}


class RDMController {
  public:
    RDMController();

    bool InitPidHelper();
    bool Setup();
    const PidStoreHelper& PidHelper() const { return m_pid_helper; }

    int PerformRequestAndWait(unsigned int universe,
                              const UID &uid,
                              uint16_t sub_device,
                              const string &pid_name,
                              bool is_set,
                              const vector<string> &inputs);

    void HandleResponse(const ola::rdm::ResponseStatus &response_status,
                        const string &rdm_data);

  private:
    typedef struct {
      unsigned int universe;
      const UID *uid;
      uint16_t sub_device;
      uint16_t pid_value;
    } pending_request_t;

    ola::OlaCallbackClientWrapper m_ola_client;
    PidStoreHelper m_pid_helper;
    pending_request_t m_pending_request;

    void FetchQueuedMessage();
    void PrintRemainingMessages(uint8_t message_count);
    void HandleAckResponse(uint16_t manufacturer_id,
                           bool is_set,
                           uint16_t pid,
                           const string &rdm_data);
};


RDMController::RDMController()
    : m_pid_helper(PID_DATA_FILE) {
}


bool RDMController::InitPidHelper() {
  return m_pid_helper.Init();
}


bool RDMController::Setup() {
  return m_ola_client.Setup();
}


/**
 * Handle the RDM response
 */
void RDMController::HandleResponse(
    const ola::rdm::ResponseStatus &response_status,
    const string &rdm_data) {
  if (!response_status.error.empty()) {
    cout << "Error: " << response_status.error << endl;
    m_ola_client.GetSelectServer()->Terminate();
    return;
  }

  if (response_status.response_code == ola::rdm::RDM_WAS_BROADCAST) {
    m_ola_client.GetSelectServer()->Terminate();
    return;
  } else if (response_status.response_code != ola::rdm::RDM_COMPLETED_OK) {
    cout << "Error: " <<
      ola::rdm::ResponseCodeToString(response_status.response_code) << endl;
    m_ola_client.GetSelectServer()->Terminate();
    return;
  }

  if (response_status.response_type == ola::rdm::RDM_ACK_TIMER) {
    m_ola_client.GetSelectServer()->RegisterSingleTimeout(
      response_status.AckTimer(),
      ola::NewSingleCallback(this, &RDMController::FetchQueuedMessage));
    return;
  }

  if (response_status.response_type == ola::rdm::RDM_ACK) {
    if (response_status.pid_value == m_pending_request.pid_value) {
      HandleAckResponse(m_pending_request.uid->ManufacturerId(),
                        response_status.set_command,
                        response_status.pid_value,
                        rdm_data);
    } else {
      // this is not the message we were looking for...
      FetchQueuedMessage();
      return;
    }
  } else if (response_status.response_type == ola::rdm::RDM_NACK_REASON) {
    cout << "Request NACKed: " <<
      ola::rdm::NackReasonToString(response_status.NackReason()) << endl;
  } else {
    cout << "Unknown RDM response type " << std::hex <<
        static_cast<int>(response_status.response_type) << endl;
  }
  PrintRemainingMessages(response_status.message_count);
  m_ola_client.GetSelectServer()->Terminate();
}


/**
 * Build a RDM Request from the options provided and send it to the daemon.
 */
int RDMController::PerformRequestAndWait(unsigned int universe,
                                         const UID &uid,
                                         uint16_t sub_device,
                                         const string &pid_name,
                                         bool is_set,
                                         const vector<string> &inputs) {
  // get the pid descriptor
  const ola::rdm::PidDescriptor *pid_descriptor = m_pid_helper.GetDescriptor(
      pid_name,
      uid.ManufacturerId());

  uint16_t pid_value;
  if (!pid_descriptor &&
      ola::PrefixedHexStringToInt(pid_name, &pid_value)) {
    pid_descriptor = m_pid_helper.GetDescriptor(
        pid_value,
        uid.ManufacturerId());
  }

  if (!pid_descriptor) {
    cout << "Unknown PID: " << pid_name << endl;
    cout << "Use --list_pids to list the available PIDs." << endl;
    return EX_USAGE;
  }

  const ola::messaging::Descriptor *descriptor = NULL;
  if (is_set)
    descriptor = pid_descriptor->SetRequest();
  else
    descriptor = pid_descriptor->GetRequest();

  if (!descriptor) {
    cout << (is_set ? "SET" : "GET") << " command not supported for "
      << pid_name << endl;
    exit(EX_USAGE);
  }

  // attempt to build the message
  auto_ptr<const ola::messaging::Message> message(m_pid_helper.BuildMessage(
      descriptor,
      inputs));

  if (!message.get()) {
    cout << m_pid_helper.SchemaAsString(descriptor);
    return EX_USAGE;
  }

  m_pending_request.universe = universe;
  m_pending_request.uid = &uid;
  m_pending_request.sub_device = sub_device;
  m_pending_request.pid_value = pid_descriptor->Value();

  unsigned int param_data_length;
  const uint8_t *param_data = m_pid_helper.SerializeMessage(
      message.get(),
      &param_data_length);

  if (is_set) {
    m_ola_client.GetClient()->RDMSet(
      ola::NewSingleCallback(this, &RDMController::HandleResponse),
      m_pending_request.universe,
      *m_pending_request.uid,
      m_pending_request.sub_device,
      pid_descriptor->Value(),
      param_data,
      param_data_length);
  } else {
    m_ola_client.GetClient()->RDMGet(
      ola::NewSingleCallback(this, &RDMController::HandleResponse),
      m_pending_request.universe,
      *m_pending_request.uid,
      m_pending_request.sub_device,
      pid_descriptor->Value(),
      param_data,
      param_data_length);
  }

  m_ola_client.GetSelectServer()->Run();
  return EX_OK;
}


/**
 * Called after the ack timer expires. This resends the request.
 */
void RDMController::FetchQueuedMessage() {
  m_ola_client.GetClient()->RDMGet(
    ola::NewSingleCallback(this, &RDMController::HandleResponse),
    m_pending_request.universe,
    *m_pending_request.uid,
    m_pending_request.sub_device,
    ola::rdm::PID_QUEUED_MESSAGE,
    NULL,
    0);
}


/**
 * Print the number of messages remaining if it is non-0.
 */
void RDMController::PrintRemainingMessages(uint8_t message_count) {
  if (!message_count)
    return;
  cout << "-----------------------------------------------------" << endl;
  cout << "Messages remaining: " << static_cast<int>(message_count) << endl;
}


/**
 * Handle an ACK response
 */
void RDMController::HandleAckResponse(uint16_t manufacturer_id,
                                      bool is_set,
                                      uint16_t pid,
                                      const string &rdm_data) {
  const ola::rdm::PidDescriptor *pid_descriptor = m_pid_helper.GetDescriptor(
      pid,
      m_pending_request.uid->ManufacturerId());

  if (!pid_descriptor) {
    OLA_WARN << "Unknown PID: " << pid << ".";
    return;
  }

  const ola::messaging::Descriptor *descriptor = NULL;
  if (is_set)
    descriptor = pid_descriptor->SetResponse();
  else
    descriptor = pid_descriptor->GetResponse();

  if (!descriptor) {
    OLA_WARN << "Unknown response message: " << (is_set ? "SET" : "GET") <<
        " " << pid_descriptor->Name();
    return;
  }

  auto_ptr<const ola::messaging::Message> message(
      m_pid_helper.DeserializeMessage(
          descriptor,
          reinterpret_cast<const uint8_t*>(rdm_data.data()),
          rdm_data.size()));

  if (!message.get()) {
    OLA_WARN << "Unable to inflate RDM response";
    return;
  }

  cout << m_pid_helper.PrettyPrintMessage(manufacturer_id,
                                          is_set,
                                          pid,
                                          message.get());
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);
  options opts;
  ParseOptions(argc, argv, &opts);
  RDMController controller;

  if (opts.help)
    DisplayHelpAndExit(opts);

  if (!controller.InitPidHelper())
    exit(EX_OSFILE);

  if (!opts.uid) {
    if (opts.list_pids) {
      DisplayPIDsAndExit(0, controller.PidHelper());
    } else {
      OLA_FATAL << "Invalid UID";
      exit(EX_USAGE);
    }
  }

  UID dest_uid(*opts.uid);
  delete opts.uid;

  if (opts.list_pids)
    DisplayPIDsAndExit(dest_uid.ManufacturerId(), controller.PidHelper());

  if (opts.args.empty())
    DisplayHelpAndExit(opts);

  if (!controller.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(EX_UNAVAILABLE);
  }

  // split out rdm message params from the pid name
  vector<string> inputs(opts.args.size() - 1);
  vector<string>::iterator args_iter = opts.args.begin();
  copy(++args_iter, opts.args.end(), inputs.begin());

  return controller.PerformRequestAndWait(opts.universe,
                                          dest_uid,
                                          opts.sub_device,
                                          opts.args[0],
                                          opts.set_mode,
                                          inputs);
}
