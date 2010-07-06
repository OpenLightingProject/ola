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
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/OlaClient.h>
#include <ola/SimpleClient.h>
#include <ola/network/SelectServer.h>
#include <ola/rdm/RDMAPI.h>
#include <ola/rdm/UID.h>


#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using std::cout;
using std::endl;
using std::string;
using std::vector;
using ola::rdm::UID;
using ola::SimpleClient;
using ola::OlaClient;
using ola::network::SelectServer;
using ola::rdm::RDMAPI;
using ola::rdm::ResponseStatus;

typedef struct {
  bool set_mode;
  bool help;       // show the help
  bool list_pids;  // show the pid list
  int uni;         // universe id
  UID *uid;         // uid
  uint16_t sub_device; // the sub device
  string pid;      // pid to get/set
  vector<string> args;  // extra args
  string cmd;  // argv[0]
} options;


class RDMController {
  public:
    RDMController(const UID *uid,
                  uint16_t sub_device,
                  RDMAPI *api,
                  SelectServer *ss):
      m_uid(*uid),
      m_sub_device(sub_device),
      m_api(api),
      m_ss(ss) {
    }

    bool GetPID(const vector<string> &tokens);
    bool SetPID(const vector<string> &tokens);

    void PrintDeviceInfo(const ResponseStatus &status,
                         const ola::rdm::DeviceInfo &device_info);

  private:
    UID m_uid;
    uint16_t m_sub_device;
    RDMAPI *m_api;
    SelectServer *m_ss;

    bool CheckForSuccess(const ResponseStatus &status);
    bool CheckForQueuedMessages();
};


/*
 * Get a pid
 */
bool RDMController::GetPID(const vector<string> &tokens) {
  return m_api->GetDeviceInfo(
      m_uid,
      m_sub_device,
      ola::NewSingleCallback(this,
                             &RDMController::PrintDeviceInfo));
}


/*
 * Set a pid
 */
bool RDMController::SetPID(const vector<string> &tokens) {
  // client->FetchUIDList(opts.uni);
  return false;
}


void RDMController::PrintDeviceInfo(
    const ResponseStatus &status,
    const ola::rdm::DeviceInfo &device_info) {
  if (!CheckForSuccess(status))
    return;

  // print device info
  OLA_INFO << "Got a response, we should really print it here";

  // check for queued messages
  if (!CheckForQueuedMessages())
    m_ss->Terminate();
}


/*
 * Check if a request completed sucessfully, if not display the errors.
 */
bool RDMController::CheckForSuccess(const ResponseStatus &status) {
  if (!status.Error().empty()) {
    cout << status.Error() << endl;
    m_ss->Terminate();
    return false;
  }

  if (status.WasNacked()) {
    // TODO(simon): print reason here
    m_ss->Terminate();
    return false;
  }
  return true;
}


bool RDMController::CheckForQueuedMessages() {
  if (!m_api->OutstandingMessagesCount(m_uid)) {
    return false;
  }

  OLA_INFO << "queued messages remain";
  // TODO(simon): query user here and then possibly print?
  return true;
}


// End RDMController implementation

/*
 * parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  opts->cmd = argv[0];
  opts->set_mode = false;
  opts->list_pids = false;
  opts->help = false;
  opts->uni = 1;
  opts->uid = NULL;
  opts->sub_device = 0;

  if (string(argv[0]) == "ola_rdm_set")
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
        opts->uni = atoi(optarg);
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
  exit(0);
}


/*
 * Dump the list of known pids
 */
void DisplayPIDsAndExit() {
  cout << "pids" << endl;
  exit(0);
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);
  SimpleClient ola_client;
  options opts;
  ParseOptions(argc, argv, &opts);

  if (opts.list_pids)
    DisplayPIDsAndExit();

  if (opts.help || opts.args.size() == 0)
    DisplayHelpAndExit(opts);

  if (!ola_client.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(1);
  }

  if (!opts.uid) {
    OLA_FATAL << "Invalid UID";
    exit(1);
  }

  SelectServer *ss = ola_client.GetSelectServer();
  RDMAPI rdm_api(opts.uni, ola_client.GetClient());

  RDMController controller(opts.uid,
                           opts.sub_device,
                           &rdm_api,
                           ss);

  bool ret = false;
  if (opts.set_mode)
    ret = controller.SetPID(opts.args);
  else
    ret = controller.GetPID(opts.args);

  if (ret)
    ss->Run();

  if (opts.uid)
    delete opts.uid;
  return 0;
}
