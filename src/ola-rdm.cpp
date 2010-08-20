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
#include <ola/OlaClient.h>
#include <ola/SimpleClient.h>
#include <ola/network/SelectServer.h>
#include <ola/rdm/RDMAPI.h>
#include <ola/rdm/UID.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "src/RDMController.h"
#include "src/RDMHandler.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;
using ola::rdm::UID;
using ola::SimpleClient;
using ola::OlaClient;
using ola::network::SelectServer;
using ola::rdm::RDMAPI;

typedef struct {
  bool set_mode;
  bool help;       // show the help
  bool list_pids;  // show the pid list
  int uni;         // universe id
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
  opts->uni = 1;
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
  exit(EX_USAGE);
}


void PopulatePidMap(map<string, uint16_t> *pid_map) {
  // populate the name -> pid map
  (*pid_map)["proxied_devices"] = ola::rdm::PID_PROXIED_DEVICES;
  (*pid_map)["proxied_device_count"] = ola::rdm::PID_PROXIED_DEVICE_COUNT;
  (*pid_map)["comms_status"] = ola::rdm::PID_COMMS_STATUS;
  (*pid_map)["queued_message"] = ola::rdm::PID_QUEUED_MESSAGE;
  (*pid_map)["status_messages"] = ola::rdm::PID_STATUS_MESSAGES;
  (*pid_map)["status_id_description"] =
    ola::rdm::PID_STATUS_ID_DESCRIPTION;
  (*pid_map)["clear_status_id"] = ola::rdm::PID_CLEAR_STATUS_ID;
  (*pid_map)["sub_device_status_report_threshold"] =
    ola::rdm::PID_SUB_DEVICE_STATUS_REPORT_THRESHOLD;
  (*pid_map)["supported_parameters"] = ola::rdm::PID_SUPPORTED_PARAMETERS;
  (*pid_map)["param_description"] = ola::rdm::PID_PARAMETER_DESCRIPTION;
  (*pid_map)["device_info"] = ola::rdm::PID_DEVICE_INFO;
  (*pid_map)["product_detail_id_list"] =
    ola::rdm::PID_PRODUCT_DETAIL_ID_LIST;
  (*pid_map)["device_model_description"] =
    ola::rdm::PID_DEVICE_MODEL_DESCRIPTION;
  (*pid_map)["manufacturer_label"] = ola::rdm::PID_MANUFACTURER_LABEL;
  (*pid_map)["device_label"] = ola::rdm::PID_DEVICE_LABEL;
  (*pid_map)["factory_defaults"] = ola::rdm::PID_FACTORY_DEFAULTS;
  (*pid_map)["language_capabilities"] =
    ola::rdm::PID_LANGUAGE_CAPABILITIES;
  (*pid_map)["language"] = ola::rdm::PID_LANGUAGE;
  (*pid_map)["software_version_label"] =
    ola::rdm::PID_SOFTWARE_VERSION_LABEL;
  (*pid_map)["boot_software_version_id"] =
    ola::rdm::PID_BOOT_SOFTWARE_VERSION_ID;
  (*pid_map)["boot_software_version_label"] =
    ola::rdm::PID_BOOT_SOFTWARE_VERSION_LABEL;
  (*pid_map)["dmx_personality"] = ola::rdm::PID_DMX_PERSONALITY;
  (*pid_map)["dmx_personality_description"] =
    ola::rdm::PID_DMX_PERSONALITY_DESCRIPTION;
  (*pid_map)["dmx_start_address"] = ola::rdm::PID_DMX_START_ADDRESS;
  (*pid_map)["slot_info"] = ola::rdm::PID_SLOT_INFO;
  (*pid_map)["slot_description"] = ola::rdm::PID_SLOT_DESCRIPTION;
  (*pid_map)["default_slot_value"] = ola::rdm::PID_DEFAULT_SLOT_VALUE;
  (*pid_map)["sensor_definition"] = ola::rdm::PID_SENSOR_DEFINITION;
  (*pid_map)["sensor_value"] = ola::rdm::PID_SENSOR_VALUE;
  (*pid_map)["record_sensors"] = ola::rdm::PID_RECORD_SENSORS;
  (*pid_map)["device_hours"] = ola::rdm::PID_DEVICE_HOURS;
  (*pid_map)["lamp_hours"] = ola::rdm::PID_LAMP_HOURS;
  (*pid_map)["lamp_strikes"] = ola::rdm::PID_LAMP_STRIKES;
  (*pid_map)["lamp_state"] = ola::rdm::PID_LAMP_STATE;
  (*pid_map)["lamp_on_mode"] = ola::rdm::PID_LAMP_ON_MODE;
  (*pid_map)["device_power_cycles"] = ola::rdm::PID_DEVICE_POWER_CYCLES;
  (*pid_map)["display_invert"] = ola::rdm::PID_DISPLAY_INVERT;
  (*pid_map)["display_level"] = ola::rdm::PID_DISPLAY_LEVEL;
  (*pid_map)["pan_invert"] = ola::rdm::PID_PAN_INVERT;
  (*pid_map)["tilt_invert"] = ola::rdm::PID_TILT_INVERT;
  (*pid_map)["pan_tilt_swap"] = ola::rdm::PID_PAN_TILT_SWAP;
  (*pid_map)["real_time_clock"] = ola::rdm::PID_REAL_TIME_CLOCK;
  (*pid_map)["identify_device"] = ola::rdm::PID_IDENTIFY_DEVICE;
  (*pid_map)["reset_device"] = ola::rdm::PID_RESET_DEVICE;
  (*pid_map)["power_state"] = ola::rdm::PID_POWER_STATE;
  (*pid_map)["perform_self_test"] = ola::rdm::PID_PERFORM_SELFTEST;
  (*pid_map)["self_test_description"] =
    ola::rdm::PID_SELF_TEST_DESCRIPTION;
  (*pid_map)["capture_preset"] = ola::rdm::PID_CAPTURE_PRESET;
  (*pid_map)["preset_playback"] = ola::rdm::PID_PRESET_PLAYBACK;
}


/*
 * Dump the list of known pids
 */
void DisplayPIDsAndExit(const map<string, uint16_t> &pid_map) {
  vector<string> pids;
  map<string, uint16_t>::const_iterator map_iter = pid_map.begin();
  for (; map_iter != pid_map.end(); ++map_iter)
    pids.push_back(map_iter->first);

  sort(pids.begin(), pids.end());
  vector<string>::const_iterator iter = pids.begin();
  for (; iter != pids.end(); ++iter) {
    std::cout << *iter << std::endl;
  }
  exit(EX_OK);
}


/*
 * Build a pid -> name map so we can nicely display pids for the user
 */
void ReversePidMap(const map<string, uint16_t> &pid_map,
                   map<uint16_t, string> *reverse_map) {
  map<string, uint16_t>::const_iterator map_iter = pid_map.begin();
  for (; map_iter != pid_map.end(); ++map_iter)
    (*reverse_map)[map_iter->second] = map_iter->first;
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  // map pid names to numbers
  map<string, uint16_t> pid_name_map;
  map<uint16_t, string> reverse_pid_name_map;
  PopulatePidMap(&pid_name_map);
  ReversePidMap(pid_name_map, &reverse_pid_name_map);

  ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);
  SimpleClient ola_client;
  options opts;
  ParseOptions(argc, argv, &opts);

  if (opts.list_pids)
    DisplayPIDsAndExit(pid_name_map);

  if (opts.help || opts.args.size() == 0)
    DisplayHelpAndExit(opts);

  if (!opts.uid) {
    OLA_FATAL << "Invalid UID";
    exit(EX_USAGE);
  }

  if (!ola_client.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(EX_UNAVAILABLE);
  }

  map<const string, uint16_t>::const_iterator name_iter =
    pid_name_map.find(opts.args[0]);

  if (name_iter == pid_name_map.end()) {
    cout << "Invalid pid name: " << opts.args[0] << endl;
    exit(EX_USAGE);
  }

  SelectServer *ss = ola_client.GetSelectServer();
  RDMAPI rdm_api(opts.uni, ola_client.GetClient());

  ResponseHandler handler(&rdm_api, ss, reverse_pid_name_map);
  RDMController controller(&rdm_api, &handler);

  string error;
  vector<string> params(opts.args.size()-1);
  copy(opts.args.begin() + 1, opts.args.end(), params.begin());
  if (controller.RequestPID(*opts.uid,
                            opts.sub_device,
                            opts.set_mode,
                            name_iter->second,
                            params,
                            &error))
    ss->Run();
  else
    std::cerr << error << endl;

  if (opts.uid)
    delete opts.uid;
  return handler.ExitCode();
}
