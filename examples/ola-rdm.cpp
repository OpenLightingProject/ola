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
 * ola-rdm.cpp
 * The command line tool for controlling RDM devices
 * Copyright (C) 2010 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/client/ClientWrapper.h>
#include <ola/client/OlaClient.h>
#include <ola/file/Util.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMAPIImplInterface.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/UID.h>
#include <ola/strings/Format.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

using ola::network::NetworkToHost;
using ola::rdm::PidStoreHelper;
using ola::rdm::PidDescriptor;
using ola::rdm::UID;
using std::auto_ptr;
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

typedef struct {
  bool set_mode;
  bool help;       // show the help
  string pid_location;  // alt pid store
  bool list_pids;  // show the pid list
  int universe;         // universe id
  UID *uid;         // uid
  uint16_t sub_device;  // the sub device
  string pid;      // pid to get/set
  vector<string> args;  // extra args
  string cmd;  // argv[0]
  bool display_frames;  // print raw frames.
} options;


/*
 * Parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  const int FRAME_OPTION_VALUE = 256;

  opts->cmd = argv[0];
  string cmd_name = ola::file::FilenameFromPathOrPath(opts->cmd);
  // To skip the lt prefix during development
  ola::StripPrefix(&cmd_name, "lt-");
#ifdef _WIN32
  // Strip the extension
  size_t extension = cmd_name.find(".");
  if (extension != string::npos) {
    cmd_name = cmd_name.substr(0, extension);
  }
#endif  // _WIN32
  opts->set_mode = false;
  opts->pid_location = "";
  opts->list_pids = false;
  opts->help = false;
  opts->universe = 1;
  opts->uid = NULL;
  opts->sub_device = 0;
  opts->display_frames = false;

  if (cmd_name == "ola_rdm_set") {
    opts->set_mode = true;
  }

  int uid_set = 0;
  static struct option long_options[] = {
      {"sub-device", required_argument, 0, 'd'},
      {"help", no_argument, 0, 'h'},
      {"pid-location", required_argument, 0, 'p'},
      {"list-pids", no_argument, 0, 'l'},
      {"universe", required_argument, 0, 'u'},
      {"frames", no_argument, 0, FRAME_OPTION_VALUE},
      {"uid", required_argument, &uid_set, 1},
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "d:hlp:u:", long_options, &option_index);

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
      case 'p':
        opts->pid_location = optarg;
        break;
      case 'u':
        opts->universe = atoi(optarg);
        break;
      case FRAME_OPTION_VALUE:
        opts->display_frames = true;
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
  cout << "Usage: " << opts.cmd <<
  " --universe <universe> --uid <uid> <pid> <value>\n"
  "\n"
  "Get the value of a PID for a device.\n"
  "Use '" << opts.cmd << " --list-pids' to get a list of applicable PIDs.\n"
  "\n"
  "  --frames                  display the raw RDM frames if available.\n"
  "  --uid <uid>               the UID of the device to control.\n"
  "  -d, --sub-device <device> target a particular sub device (default is 0)\n"
  "  -h, --help                display this help message and exit.\n"
  "  -l, --list-pids           display a list of PIDs that support GET.\n"
  "  -p, --pid-location        the directory to read PID definitions from\n"
  "  -u, --universe <universe> universe number.\n"
  << endl;
}


/*
 * Display the help for set_pid
 */
void DisplaySetPidHelp(const options &opts) {
  cout << "Usage: " << opts.cmd <<
  " --universe <universe> --uid <uid> <pid> <value>\n"
  "\n"
  "Set the value of a PID for a device.\n"
  "Use '" << opts.cmd << " --list-pids' to get a list of applicable PIDs.\n"
  "\n"
  "  --frames                  display the raw RDM frames if available.\n"
  "  --uid <uid>               the UID of the device to control.\n"
  "  -d, --sub-device <device> target a particular sub device (default is 0)\n"
  "  -h, --help                display this help message and exit.\n"
  "  -l, --list-pids           display a list of PIDs that support SET.\n"
  "  -p, --pid-location        the directory to read PID definitions from\n"
  "  -u, --universe <universe> universe number.\n"
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
  exit(ola::EXIT_USAGE);
}


/*
 * Dump the list of known pids
 */
void DisplayPIDsAndExit(uint16_t manufacturer_id,
                        const PidStoreHelper &pid_helper,
                        bool set_mode) {
  vector<const PidDescriptor *> pids;
  pid_helper.SupportedPids(manufacturer_id, &pids);
  std::sort(pids.begin(), pids.end(), &PidDescriptor::OrderByName);

  // Remove PIDs that don't support the proper command class (GET with
  // ola_rdm_get, SET with ola_rdm_set)
  vector<const PidDescriptor *>::const_iterator it_pids = pids.begin();
  for (; it_pids != pids.end(); ++it_pids) {
    if ((set_mode && (*it_pids)->SetRequest() != NULL) ||
        (!set_mode && (*it_pids)->GetRequest() != NULL)) {
      cout << (*it_pids)->Name() << endl;
    }
  }
  exit(ola::EXIT_OK);
}


class RDMController {
 public:
  RDMController(string pid_location, bool show_frames);

  bool InitPidHelper();
  bool Setup();
  const PidStoreHelper& PidHelper() const { return m_pid_helper; }

  int PerformRequestAndWait(unsigned int universe,
                            const UID &uid,
                            uint16_t sub_device,
                            const string &pid_name,
                            bool is_set,
                            const vector<string> &inputs);

  void HandleResponse(const ola::client::Result &result,
                      const ola::client::RDMMetadata &metadata,
                      const ola::rdm::RDMResponse *response);

  void ShowFrames(const ola::client::RDMMetadata &metadata);

 private:
  struct PendingRequest {
   public:
    PendingRequest()
        : universe(0),
          uid(NULL),
          sub_device(0),
          pid_value(0) {
    }

    unsigned int universe;
    const UID *uid;
    uint16_t sub_device;
    uint16_t pid_value;
  };

  const bool m_show_frames;
  ola::client::OlaClientWrapper m_ola_client;
  PidStoreHelper m_pid_helper;
  PendingRequest m_pending_request;

  void FetchQueuedMessage();
  void PrintRemainingMessages(uint8_t message_count);
  void HandleAckResponse(uint16_t manufacturer_id,
                         bool is_set,
                         uint16_t pid,
                         const uint8_t *data,
                         unsigned int length);
};


RDMController::RDMController(string pid_location, bool show_frames)
    : m_show_frames(show_frames),
      m_pid_helper(pid_location) {
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
void RDMController::HandleResponse(const ola::client::Result &result,
                                   const ola::client::RDMMetadata &metadata,
                                   const ola::rdm::RDMResponse *response) {
  if (!result.Success()) {
    cerr << "Error: " << result.Error() << endl;
    ShowFrames(metadata);
    m_ola_client.GetSelectServer()->Terminate();
    return;
  }

  if (metadata.response_code == ola::rdm::RDM_WAS_BROADCAST) {
    m_ola_client.GetSelectServer()->Terminate();
    // Broadcast, there shouldn't be any frames
    return;
  } else if (metadata.response_code != ola::rdm::RDM_COMPLETED_OK) {
    cerr << "Error: "
         << ola::rdm::StatusCodeToString(metadata.response_code) << endl;
    ShowFrames(metadata);
    m_ola_client.GetSelectServer()->Terminate();
    return;
  }

  if (!response) {
    cerr << "Error: Missing RDM Response but response_code was "
            "RDM_COMPLETED_OK, this is a bug, please report it!"
         << endl;
    ShowFrames(metadata);
    return;
  }

  if (response->ResponseType() == ola::rdm::RDM_ACK_TIMER) {
    uint16_t backoff_time;
    if (response->ParamDataSize() != sizeof(backoff_time)) {
      cerr << "Invalid ACK_TIMER param size of " << response->ParamDataSize();
    } else {
      memcpy(reinterpret_cast<uint8_t*>(&backoff_time), response->ParamData(),
             sizeof(backoff_time));
      unsigned int timeout = 100 * NetworkToHost(backoff_time);
      m_ola_client.GetSelectServer()->RegisterSingleTimeout(
        timeout,
        ola::NewSingleCallback(this, &RDMController::FetchQueuedMessage));
    }
  } else if (response->ResponseType() == ola::rdm::RDM_ACK) {
    if (response->ParamId() == m_pending_request.pid_value ||
        m_pending_request.pid_value == ola::rdm::PID_QUEUED_MESSAGE) {
      HandleAckResponse(
          m_pending_request.uid->ManufacturerId(),
          response->CommandClass() ==
              ola::rdm::RDMCommand::SET_COMMAND_RESPONSE,
          response->ParamId(),
          response->ParamData(),
          response->ParamDataSize());
    } else {
      // we got something other than an empty status message, this means there
      // there are probably more messages to fetch
      if (response->ParamId() != ola::rdm::PID_STATUS_MESSAGES ||
          response->ParamDataSize() != 0) {
        FetchQueuedMessage();
        return;
      }
      // this is just an empty status message, the device probably doesn't
      // support queued messages.
      cout << "Empty STATUS_MESSAGES returned." << endl;
    }
  } else if (response->ResponseType() == ola::rdm::RDM_NACK_REASON) {
    uint16_t nack_reason;
    if (response->ParamDataSize() != sizeof(nack_reason)) {
      cerr << "Invalid NACK reason size of " << response->ParamDataSize();
    } else {
      memcpy(reinterpret_cast<uint8_t*>(&nack_reason), response->ParamData(),
             sizeof(nack_reason));
      nack_reason = NetworkToHost(nack_reason);
      cout << "Request NACKed: " <<
        ola::rdm::NackReasonToString(nack_reason) << endl;
    }
  } else {
    cout << "Unknown RDM response type "
         << ola::strings::ToHex(response->ResponseType()) << endl;
  }
  PrintRemainingMessages(response->MessageCount());

  ShowFrames(metadata);
  m_ola_client.GetSelectServer()->Terminate();
}


/**
 * Show frames if asked for
 */
void RDMController::ShowFrames(const ola::client::RDMMetadata &metadata) {
  if (m_show_frames && !metadata.frames.empty()) {
    cout << "------- Frame Information --------" << endl;
    ola::rdm::RDMFrames::const_iterator iter = metadata.frames.begin();
    for (; iter != metadata.frames.end(); ++iter) {
      std::ios::fmtflags f(cout.flags());
      cout << std::fixed << std::setprecision(1);

      if (iter->timing.response_time) {
        cout << "Response Time: " << iter->timing.response_time / 1000.0 << "uS"
             << endl;
      }
      if (iter->timing.break_time) {
        cout << "Break Time: " << iter->timing.break_time / 1000.0 << "uS"
             << endl;
      }
      if (iter->timing.mark_time) {
        cout << "Mark Time: " << iter->timing.mark_time / 1000.0 << "uS"
             << endl;
      }
      if (iter->timing.data_time) {
        cout << "Data Time: " << iter->timing.data_time / 1000.0 << "uS"
             << endl;
      }
      cout.flags(f);
      ola::strings::FormatData(&cout, iter->data.data(), iter->data.size());
    }
  }
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
      (ola::PrefixedHexStringToInt(pid_name, &pid_value) ||
       ola::StringToInt(pid_name, &pid_value))) {
    pid_descriptor = m_pid_helper.GetDescriptor(
        pid_value,
        uid.ManufacturerId());
  }

  if (!pid_descriptor) {
    cout << "Unknown PID: " << pid_name << endl;
    cout << "Use --list-pids to list the available PIDs." << endl;
    return ola::EXIT_USAGE;
  }

  const ola::messaging::Descriptor *descriptor = NULL;
  if (is_set)
    descriptor = pid_descriptor->SetRequest();
  else
    descriptor = pid_descriptor->GetRequest();

  if (!descriptor) {
    cout << (is_set ? "SET" : "GET") << " command not supported for "
      << pid_name << endl;
    exit(ola::EXIT_USAGE);
  }

  // attempt to build the message
  auto_ptr<const ola::messaging::Message> message(m_pid_helper.BuildMessage(
      descriptor,
      inputs));

  if (!message.get()) {
    cout << m_pid_helper.SchemaAsString(descriptor);
    return ola::EXIT_USAGE;
  }

  m_pending_request.universe = universe;
  m_pending_request.uid = &uid;
  m_pending_request.sub_device = sub_device;
  m_pending_request.pid_value = pid_descriptor->Value();

  unsigned int param_data_length;
  const uint8_t *param_data = m_pid_helper.SerializeMessage(
      message.get(),
      &param_data_length);

  ola::client::SendRDMArgs args(
      ola::NewSingleCallback(this, &RDMController::HandleResponse));
  if (m_show_frames) {
    args.include_raw_frames = true;
  }

  if (is_set) {
    m_ola_client.GetClient()->RDMSet(
      m_pending_request.universe,
      *m_pending_request.uid,
      m_pending_request.sub_device,
      pid_descriptor->Value(),
      param_data,
      param_data_length,
      args);
  } else {
    m_ola_client.GetClient()->RDMGet(
      m_pending_request.universe,
      *m_pending_request.uid,
      m_pending_request.sub_device,
      pid_descriptor->Value(),
      param_data,
      param_data_length,
      args);
  }

  m_ola_client.GetSelectServer()->Run();
  return ola::EXIT_OK;
}


/**
 * Called after the ack timer expires. This resends the request.
 */
void RDMController::FetchQueuedMessage() {
  uint8_t status_type = 4;
  ola::client::SendRDMArgs args(
      ola::NewSingleCallback(this, &RDMController::HandleResponse));

  m_ola_client.GetClient()->RDMGet(
    m_pending_request.universe,
    *m_pending_request.uid,
    m_pending_request.sub_device,
    ola::rdm::PID_QUEUED_MESSAGE,
    &status_type,
    sizeof(status_type),
    args);
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
                                      const uint8_t *data,
                                      unsigned int length) {
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
      m_pid_helper.DeserializeMessage(descriptor, data, length));

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
  if (!ola::NetworkInit()) {
    OLA_WARN << "Network initialization failed." << endl;
    exit(ola::EXIT_UNAVAILABLE);
  }
  options opts;
  ParseOptions(argc, argv, &opts);
  RDMController controller(opts.pid_location, opts.display_frames);

  if (opts.help)
    DisplayHelpAndExit(opts);

  // Make sure we can load our PIDs
  if (!controller.InitPidHelper())
    exit(ola::EXIT_OSFILE);

  if (!opts.uid) {
    if (opts.list_pids) {
      DisplayPIDsAndExit(0, controller.PidHelper(), opts.set_mode);
    } else {
      OLA_FATAL << "Invalid or missing UID, try xxxx:yyyyyyyy";
      DisplayHelpAndExit(opts);
    }
  }

  UID dest_uid(*opts.uid);
  delete opts.uid;

  if (opts.list_pids)
    DisplayPIDsAndExit(dest_uid.ManufacturerId(),
                       controller.PidHelper(),
                       opts.set_mode);

  if (opts.args.empty())
    DisplayHelpAndExit(opts);

  if (!controller.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(ola::EXIT_UNAVAILABLE);
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
