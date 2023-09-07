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
 * ola-client.cpp
 * The multi purpose ola client.
 * Copyright (C) 2005 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/client/ClientWrapper.h>
#include <ola/client/OlaClient.h>
#include <ola/file/Util.h>
#include <ola/io/SelectServer.h>
#include <olad/PortConstants.h>


#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using ola::NewSingleCallback;
using ola::client::OlaClient;
using ola::client::OlaClientWrapper;
using ola::client::OlaDevice;
using ola::client::OlaInputPort;
using ola::client::OlaOutputPort;
using ola::client::OlaPlugin;
using ola::client::OlaUniverse;
using ola::client::Result;
using ola::io::SelectServer;
using std::cerr;
using std::cout;
using std::endl;
using std::setw;
using std::string;
using std::vector;

static const int INVALID_VALUE = -1;

/*
 * The mode is determined by the name in which we were called
 */
typedef enum {
  DEVICE_INFO,
  DEVICE_PATCH,
  PLUGIN_INFO,
  PLUGIN_STATE,
  UNIVERSE_INFO,
  UNIVERSE_NAME,
  UNI_MERGE,
  SET_DMX,
  SET_PORT_PRIORITY,
} mode;


typedef struct {
  mode m;          // mode
  int uni;         // universe id
  unsigned int plugin_id;   // plugin id
  bool help;       // show the help
  int device_id;   // device id
  int port_id;     // port id
  ola::client::PortDirection port_direction;  // input or output
  ola::client::PatchAction patch_action;      // patch or unpatch
  OlaUniverse::merge_mode merge_mode;  // the merge mode
  string cmd;      // argv[0]
  string uni_name;  // universe name
  string dmx;      // DMX string
  bool blackout;
  ola::port_priority_mode priority_mode;  // port priority mode
  uint8_t priority_value;  // port priority value
  bool list_plugin_ids;
  bool list_universe_ids;
  string state;      // plugin enable/disable state
} options;


/**
 * A Helper function to display a list of ports
 */
template<class PortClass>
void ListPorts(const vector<PortClass> &ports, bool input) {
  typename vector<PortClass>::const_iterator port_iter;
  for (port_iter = ports.begin(); port_iter != ports.end(); ++port_iter) {
    cout << "  port " << port_iter->Id() << ", ";

    if (input) {
      cout << "IN";
    } else {
      cout << "OUT";
    }

    if (!port_iter->Description().empty()) {
      cout << " " << port_iter->Description();
    }

    switch (port_iter->PriorityCapability()) {
      case ola::CAPABILITY_STATIC:
        cout << ", priority " << static_cast<int>(port_iter->Priority());
        break;
      case ola::CAPABILITY_FULL:
        cout << ", priority ";
        if (port_iter->PriorityMode() == ola::PRIORITY_MODE_INHERIT) {
          cout << "inherited";
        } else {
          cout << "override " << static_cast<int>(port_iter->Priority());
        }
        break;
      default:
        break;
    }

    if (port_iter->IsActive()) {
      cout << ", patched to universe " << port_iter->Universe();
    }

    if (port_iter->SupportsRDM()) {
      cout << ", RDM supported";
    }
    cout << endl;
  }
}


/*
 * This is called when we receive universe results from the client
 * @param list_ids_only show ids only
 * @param universes a vector of OlaUniverses
 */
void DisplayUniverses(SelectServer *ss,
                      bool list_ids_only,
                      const Result &result,
                      const vector <OlaUniverse> &universes) {
  vector<OlaUniverse>::const_iterator iter;

  if (!result.Success()) {
    cerr << result.Error() << endl;
    ss->Terminate();
    return;
  }

  if (list_ids_only) {
    for (iter = universes.begin(); iter != universes.end(); ++iter) {
      cout << iter->Id() << endl;
    }
  } else {
    cout << setw(5) << "Id" << "\t" << setw(30) << "Name" << "\t\tMerge Mode"
         << endl;
    cout << "----------------------------------------------------------"
         << endl;

    for (iter = universes.begin(); iter != universes.end(); ++iter) {
      cout << setw(5) << iter->Id() << "\t" << setw(30) << iter->Name()
           << "\t\t"
           << (iter->MergeMode() == OlaUniverse::MERGE_HTP ? "HTP" : "LTP")
           << endl;
    }

    cout << "----------------------------------------------------------" <<
      endl;
  }

  ss->Terminate();
}


/*
 * @param list_ids_only show ids only
 * @params plugins a vector of OlaPlugins
 */
void DisplayPlugins(SelectServer *ss,
                    bool list_ids_only,
                    const Result &result,
                    const vector <OlaPlugin> &plugins) {
  vector<OlaPlugin>::const_iterator iter;

  if (!result.Success()) {
    cerr << result.Error() << endl;
    ss->Terminate();
    return;
  }

  if (list_ids_only) {
    for (iter = plugins.begin(); iter != plugins.end(); ++iter) {
      cout << iter->Id() << endl;
    }
  } else {
    cout << setw(5) << "Id" << "\tPlugin Name" << endl;
    cout << "--------------------------------------" << endl;

    for (iter = plugins.begin(); iter != plugins.end(); ++iter) {
      cout << setw(5) << iter->Id() << "\t" << iter->Name() << endl;
    }

    cout << "--------------------------------------" << endl;
  }

  ss->Terminate();
}


/*
 * Print a plugin description
 */
void DisplayPluginDescription(SelectServer *ss,
                              const Result &result,
                              const string &description) {
  if (!result.Success()) {
    cerr << result.Error() << endl;
  } else {
    cout << description << endl;
  }
  ss->Terminate();
  return;
}


/*
 * Print a plugin state
 */
void DisplayPluginState(SelectServer *ss,
                        const Result &result,
                        const ola::client::PluginState &state) {
  if (!result.Success()) {
    cerr << result.Error() << endl;
  } else {
    cout << state.name << endl;
    cout << "Enabled: " << (state.enabled ? "True" : "False") << endl;
    cout << "Active: " << (state.active ? "True" : "False") << endl;
    vector<OlaPlugin>::const_iterator iter = state.conflicting_plugins.begin();
    cout << "Conflicts with:" << endl;
    for (; iter != state.conflicting_plugins.end(); ++iter) {
      cout << "  " << iter->Name() << "(" << iter->Id() << ")" << endl;
    }
  }
  ss->Terminate();
  return;
}


/*
 * @param devices a vector of OlaDevices
 */
void DisplayDevices(SelectServer *ss,
                    const Result &result,
                    const vector <OlaDevice> &devices) {
  vector<OlaDevice>::const_iterator iter;

  if (!result.Success()) {
    cerr << result.Error() << endl;
    ss->Terminate();
    return;
  }

  for (iter = devices.begin(); iter != devices.end(); ++iter) {
    cout << "Device " << iter->Alias() << ": " << iter->Name() << endl;
    vector<OlaInputPort> input_ports = iter->InputPorts();
    ListPorts(input_ports, true);
    vector<OlaOutputPort> output_ports = iter->OutputPorts();
    ListPorts(output_ports, false);
  }
  ss->Terminate();
}

/*
 * Called when a generic set command completes
 */
void HandleAck(SelectServer *ss, const Result &result) {
  if (!result.Success()) {
    cerr << result.Error() << endl;
  }
  ss->Terminate();
}

/*
 * Init options
 */
void InitOptions(options *opts) {
  opts->m = DEVICE_INFO;
  opts->uni = INVALID_VALUE;
  opts->plugin_id = ola::OLA_PLUGIN_ALL;
  opts->help = false;
  opts->list_plugin_ids = false;
  opts->list_universe_ids = false;
  opts->patch_action = ola::client::PATCH;
  opts->port_id = INVALID_VALUE;
  opts->port_direction = ola::client::OUTPUT_PORT;
  opts->device_id = INVALID_VALUE;
  opts->merge_mode = OlaUniverse::MERGE_HTP;
  opts->blackout = false;
  opts->priority_mode = ola::PRIORITY_MODE_INHERIT;
  opts->priority_value = 0;
}


/*
 * Decide what mode we're running in
 */
void SetMode(options *opts) {
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

  if (cmd_name == "ola_plugin_info") {
    opts->m = PLUGIN_INFO;
  } else if (cmd_name == "ola_plugin_state") {
    opts->m = PLUGIN_STATE;
  } else if (cmd_name == "ola_patch") {
    opts->m = DEVICE_PATCH;
  } else if (cmd_name == "ola_ptch") {
    // Working around Windows UAC
    opts->m = DEVICE_PATCH;
  } else if (cmd_name == "ola_uni_info") {
    opts->m = UNIVERSE_INFO;
  } else if (cmd_name == "ola_uni_name") {
    opts->m = UNIVERSE_NAME;
  } else if (cmd_name == "ola_uni_merge") {
    opts->m = UNI_MERGE;
  } else if (cmd_name == "ola_set_dmx") {
    opts->m = SET_DMX;
  } else if (cmd_name == "ola_set_priority") {
    opts->m = SET_PORT_PRIORITY;
  }
}


/*
 * parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  enum {
    LIST_PLUGIN_IDS_OPTION = 256,
    LIST_UNIVERSE_IDS_OPTION,
  };

  static struct option long_options[] = {
      {"dmx", required_argument, 0, 'd'},
      {"blackout", no_argument, 0, 'b'},
      {"help", no_argument, 0, 'h'},
      {"ltp", no_argument, 0, 'l'},
      {"name", required_argument, 0, 'n'},
      {"plugin-id", required_argument, 0, 'p'},
      {"state", required_argument, 0, 's'},
      {"list-plugin-ids", no_argument, 0, LIST_PLUGIN_IDS_OPTION},
      {"list-universe-ids", no_argument, 0, LIST_UNIVERSE_IDS_OPTION},
      {"universe", required_argument, 0, 'u'},
      {0, 0, 0, 0}
    };

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "ld:bn:u:p:s:hv", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'd':
        opts->dmx = optarg;
        break;
      case 'b':
        opts->blackout = true;
        break;
      case 'h':
        opts->help = true;
        break;
      case 'l':
        opts->merge_mode = OlaUniverse::MERGE_LTP;
        break;
      case 'n':
        opts->uni_name = optarg;
        break;
      case 'p':
        opts->plugin_id = atoi(optarg);
        break;
      case 's':
        opts->state = optarg;
        break;
      case 'u':
        opts->uni = atoi(optarg);
        break;
      case LIST_PLUGIN_IDS_OPTION:
        opts->list_plugin_ids = true;
        break;
      case LIST_UNIVERSE_IDS_OPTION:
        opts->list_universe_ids = true;
        break;
      case '?':
        break;
      default:
        break;
    }
  }
}


/*
 * parse our cmd line options for the patch command
 */
int ParsePatchOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"device", required_argument, 0, 'd'},
      {"help", no_argument, 0, 'h'},
      {"input", no_argument, 0, 'i'},
      {"patch", no_argument, 0, 'a'},
      {"port", required_argument, 0, 'p'},
      {"universe", required_argument, 0, 'u'},
      {"unpatch", no_argument, 0, 'r'},
      {0, 0, 0, 0}
    };

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "ard:p:u:hi", long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
      case 0:
        break;
      case 'a':
        opts->patch_action = ola::client::PATCH;
        break;
      case 'd':
        opts->device_id = atoi(optarg);
        break;
      case 'p':
        opts->port_id = atoi(optarg);
        break;
      case 'r':
        opts->patch_action = ola::client::UNPATCH;
        break;
      case 'u':
        opts->uni = atoi(optarg);
        break;
      case 'h':
        opts->help = true;
        break;
      case 'i':
        opts->port_direction = ola::client::INPUT_PORT;
        break;
      case '?':
        break;
      default:
        break;
    }
  }
  return 0;
}


/*
 * parse our cmd line options for the set priority command
 */
int ParseSetPriorityOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"device", required_argument, 0, 'd'},
      {"help", no_argument, 0, 'h'},
      {"input", no_argument, 0, 'i'},
      {"port", required_argument, 0, 'p'},
      {"override", required_argument, 0, 'o'},
      {0, 0, 0, 0}
    };

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "d:p:o:hi", long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
      case 0:
        break;
      case 'd':
        opts->device_id = atoi(optarg);
        break;
      case 'h':
        opts->help = true;
        break;
      case 'i':
        opts->port_direction = ola::client::INPUT_PORT;
        break;
      case 'o':
        opts->priority_mode = ola::PRIORITY_MODE_STATIC;
        opts->priority_value = atoi(optarg);
        break;
      case 'p':
        opts->port_id = atoi(optarg);
        break;
      case '?':
        break;
      default:
        break;
    }
  }
  return 0;
}


/*
 * help message for device info
 */
void DisplayDeviceInfoHelp(const options &opts) {
  cout << "Usage: " << opts.cmd << " [--plugin-id <plugin_id>]\n"
          "\n"
          "Show information on the devices loaded by olad.\n"
          "\n"
          "  -h, --help                  Display this help message and exit.\n"
          "  -p, --plugin-id <plugin-id> Show only devices owned by this "
          "plugin.\n"
       << endl;
}


/*
 * Display the Patch help
 */
void DisplayPatchHelp(const options &opts) {
  cout << "Usage: " << opts.cmd
       << " [--patch | --unpatch] --device <dev> --port <port> "
          "[--universe <uni>]\n"
          "\n"
          "Control ola port <-> universe mappings.\n"
          "\n"
          "  -a, --patch              Patch this port (default).\n"
          "  -d, --device <device>    Id of device to patch.\n"
          "  -h, --help               Display this help message and exit.\n"
          "  -p, --port <port>        Id of the port to patch.\n"
          "  -r, --unpatch            Unpatch this port.\n"
          "  -i, --input              Patch the input port (default is "
          "output).\n"
          "  -u, --universe <uni>     Id of the universe to patch to (default "
          "0).\n"
       << endl;
}


/*
 * help message for plugin info
 */
void DisplayPluginInfoHelp(const options &opts) {
  cout << "Usage: " << opts.cmd << " [--plugin-id <plugin-id>]\n"
          "\n"
          "Get info on the plugins loaded by olad. Called without arguments"
          " this will\n"
          "display the plugins loaded by olad. When used with --plugin-id this"
          " will\n"
          "display the specified plugin's description.\n"
          "\n"
          "  -h, --help                  Display this help message and exit.\n"
          "  -p, --plugin-id <plugin_id> Id of the plugin to fetch the "
          "description of\n"
          "  --list-plugin-ids           List plugin Ids only.\n"
       << endl;
}


/*
 * help message for plugin state
 */
void DisplayPluginStateHelp(const options &opts) {
  cout << "Usage: " << opts.cmd
       << " --plugin-id <plugin-id> [--state <enable|disable>]\n"
          "\n"
          "Displays the enabled/disabled state for a plugin and the list of "
          "plugins\n"
          "this plugin will conflict with.\n"
          "\n"
          "  -h, --help                  Display this help message and exit.\n"
          "  -p, --plugin-id <plugin-id> Id of the plugin to fetch the state "
          "of\n"
          "  -s, --state <enable|disable> State to set a plugin to\n"
      << endl;
}


/*
 * help message for uni info
 */
void DisplayUniverseInfoHelp(const options &opts) {
  cout << "Usage: " << opts.cmd
       << "\n"
          "Shows info on the active universes in use.\n"
          "\n"
          "  -h, --help          Display this help message and exit.\n"
          "  --list-universe-ids List universe Ids only.\n"
       << endl;
}


/*
 * Help message for set uni name
 */
void DisplayUniverseNameHelp(const options &opts) {
  cout << "Usage: " << opts.cmd << " --name <name> --universe <uni>\n"
          "\n"
          "Set a name for the specified universe\n"
          "\n"
          "  -h, --help                Display this help message and exit.\n"
          "  -n, --name <name>         Name for the universe.\n"
          "  -u, --universe <universe> Id of the universe to name.\n"
       << endl;
}


/*
 * Help message for set uni merge mode
 */
void DisplayUniverseMergeHelp(const options &opts) {
  cout << "Usage: " << opts.cmd << " --universe <uni> [--ltp]\n"
          "\n"
          "Change the merge mode for the specified universe. Without --ltp "
          "it will\n"
          "revert to HTP mode.\n"
          "\n"
          "  -h, --help                Display this help message and exit.\n"
          "  -l, --ltp                 Change to LTP mode.\n"
          "  -u, --universe <universe> Id of the universe to change.\n"
       << endl;
}



/*
 * Help message for set dmx
 */
void DisplaySetDmxHelp(const options &opts) {
  cout << "Usage: " << opts.cmd << " --universe <universe> [ --dmx <values> ] "
          "[ --blackout ]\n"
          "\n"
          "Sets the DMX values for a universe.\n"
          "\n"
          "  -h, --help                Display this help message and exit.\n"
          "  -u, --universe <universe> Universe number, e.g. 0.\n"
          "  -d, --dmx <values>        Comma separated DMX values, e.g. "
          "0,255,128 sets first channel to 0, second channel to 255"
          " and third channel to 128.\n"
          "  -b, --blackout            Send a universe to blackout instead.\n"
       << endl;
}

/*
 * Display the Patch help
 */
void DisplaySetPriorityHelp(const options &opts) {
  cout << "Usage: " << opts.cmd
       << " --device <dev> --port <port> [--override <value>]\n"
          "\n"
          "Set a port's priority, without the --override flag this will set "
          "the port\n"
          " to inherit mode.\n"
          "\n"
          "  -d, --device <device>    Id of device to set priority for.\n"
          "  -h, --help               Display this help message and exit.\n"
          "  -i, --input              Set an input port\n"
          "  -o, --override <value>   Set the port priority to a static "
          "value.\n"
          "  -p, --port <port>        Id of the port to set priority for.\n"
       << endl;
}



/*
 * Display the help message
 */
void DisplayHelpAndExit(const options &opts) {
  switch (opts.m) {
    case DEVICE_INFO:
      DisplayDeviceInfoHelp(opts);
      break;
    case DEVICE_PATCH:
      DisplayPatchHelp(opts);
      break;
    case PLUGIN_INFO:
      DisplayPluginInfoHelp(opts);
      break;
    case PLUGIN_STATE:
      DisplayPluginStateHelp(opts);
      break;
    case UNIVERSE_INFO:
      DisplayUniverseInfoHelp(opts);
      break;
    case UNIVERSE_NAME:
      DisplayUniverseNameHelp(opts);
      break;
    case UNI_MERGE:
      DisplayUniverseMergeHelp(opts);
      break;
    case SET_DMX:
      DisplaySetDmxHelp(opts);
      break;
    case SET_PORT_PRIORITY:
      DisplaySetPriorityHelp(opts);
  }
  exit(0);
}


/*
 * Send a fetch device info request
 * @param client  the ola client
 * @param opts  the const options
 */
int FetchDeviceInfo(OlaClientWrapper *wrapper, const options &opts) {
  SelectServer *ss = wrapper->GetSelectServer();
  OlaClient *client = wrapper->GetClient();
  client->FetchDeviceInfo((ola::ola_plugin_id) opts.plugin_id,
                          NewSingleCallback(&DisplayDevices, ss));
  return 0;
}


void Patch(OlaClientWrapper *wrapper, const options &opts) {
  SelectServer *ss = wrapper->GetSelectServer();
  OlaClient *client = wrapper->GetClient();
  if (opts.device_id == INVALID_VALUE || opts.port_id == INVALID_VALUE) {
    DisplayPatchHelp(opts);
    exit(1);
  }

  if (opts.patch_action == ola::client::PATCH  && opts.uni == INVALID_VALUE) {
    DisplayPatchHelp(opts);
    exit(1);
  }
  client->Patch(opts.device_id,
                opts.port_id,
                opts.port_direction,
                opts.patch_action, opts.uni,
                NewSingleCallback(&HandleAck, ss));
}


/*
 * Fetch information on plugins.
 */
int FetchPluginInfo(OlaClientWrapper *wrapper, const options &opts) {
  SelectServer *ss = wrapper->GetSelectServer();
  OlaClient *client = wrapper->GetClient();
  if (opts.plugin_id > 0) {
    client->FetchPluginDescription(
        (ola::ola_plugin_id) opts.plugin_id,
        NewSingleCallback(&DisplayPluginDescription, ss));
  } else {
    client->FetchPluginList(
        NewSingleCallback(&DisplayPlugins, ss, opts.list_plugin_ids));
  }
  return 0;
}


/*
 * Fetch the state of a plugin.
 */
int FetchPluginState(OlaClientWrapper *wrapper, const options &opts) {
  SelectServer *ss = wrapper->GetSelectServer();
  OlaClient *client = wrapper->GetClient();
  if (opts.plugin_id == 0) {
    DisplayPluginStateHelp(opts);
    exit(1);
  }
  if (!opts.state.empty()) {
    bool state;
    if (ola::StringToBoolTolerant(opts.state, &state)) {
      cout << "Setting state to " << (state ? "enabled" : "disabled") << endl;
      client->SetPluginState(
          (ola::ola_plugin_id) opts.plugin_id,
          state,
          NewSingleCallback(&HandleAck, ss));
    } else {
      cerr << "Invalid state: " << opts.state << endl;
      DisplayPluginStateHelp(opts);
      exit(1);
    }
  } else {
    client->FetchPluginState((ola::ola_plugin_id) opts.plugin_id,
                             NewSingleCallback(&DisplayPluginState, ss));
  }
  return 0;
}



/*
 * send a set name request
 * @param client the ola client
 * @param opts  the const options
 */
int SetUniverseName(OlaClientWrapper *wrapper, const options &opts) {
  SelectServer *ss = wrapper->GetSelectServer();
  OlaClient *client = wrapper->GetClient();
  if (opts.uni == INVALID_VALUE) {
    DisplayUniverseNameHelp(opts);
    exit(1);
  }
  client->SetUniverseName(opts.uni, opts.uni_name,
                          NewSingleCallback(&HandleAck, ss));
  return 0;
}


/*
 * send a set name request
 * @param client the ola client
 * @param opts  the const options
 */
int SetUniverseMergeMode(OlaClientWrapper *wrapper,
                         const options &opts) {
  SelectServer *ss = wrapper->GetSelectServer();
  OlaClient *client = wrapper->GetClient();
  if (opts.uni == INVALID_VALUE) {
    DisplayUniverseMergeHelp(opts);
    exit(1);
  }
  client->SetUniverseMergeMode(
      opts.uni, opts.merge_mode,
      NewSingleCallback(&HandleAck, ss));
  return 0;
}


/*
 * Send a DMX message
 * @param client the ola client
 * @param opts the options
 */
int SendDmx(OlaClientWrapper *wrapper, const options &opts) {
  SelectServer *ss = wrapper->GetSelectServer();
  OlaClient *client = wrapper->GetClient();
  ola::DmxBuffer buffer;
  bool status = false;
  if (opts.blackout) {
    status = buffer.Blackout();
  } else {
    status = buffer.SetFromString(opts.dmx);
  }

  // A dmx string and blackout are mutually exclusive
  if (opts.uni < 0 || !status || (opts.blackout && !opts.dmx.empty()) ||
      buffer.Size() == 0) {
    DisplaySetDmxHelp(opts);
    exit(1);
  }

  ola::client::SendDMXArgs args(NewSingleCallback(&HandleAck, ss));
  client->SendDMX(opts.uni, buffer, args);
  return 0;
}


/*
 * Set the priority of a port
 */
void SetPortPriority(OlaClientWrapper *wrapper, const options &opts) {
  SelectServer *ss = wrapper->GetSelectServer();
  OlaClient *client = wrapper->GetClient();

  if (opts.device_id == INVALID_VALUE || opts.port_id == INVALID_VALUE) {
    DisplaySetPriorityHelp(opts);
    exit(1);
  }

  if (opts.priority_mode == ola::PRIORITY_MODE_INHERIT) {
    client->SetPortPriorityInherit(
        opts.device_id, opts.port_id, opts.port_direction,
        NewSingleCallback(&HandleAck, ss));
  } else if (opts.priority_mode == ola::PRIORITY_MODE_STATIC) {
    client->SetPortPriorityOverride(
        opts.device_id, opts.port_id, opts.port_direction, opts.priority_value,
        NewSingleCallback(&HandleAck, ss));
  } else {
    DisplaySetPriorityHelp(opts);
  }
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
  OlaClientWrapper ola_client;
  options opts;

  InitOptions(&opts);
  opts.cmd = argv[0];

  // decide how we should behave
  SetMode(&opts);

  if (opts.m == DEVICE_PATCH) {
    ParsePatchOptions(argc, argv, &opts);
  } else if (opts.m == SET_PORT_PRIORITY) {
    ParseSetPriorityOptions(argc, argv, &opts);
  } else {
    ParseOptions(argc, argv, &opts);
  }

  if (opts.help) {
    DisplayHelpAndExit(opts);
  }

  if (!ola_client.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(1);
  }

  switch (opts.m) {
    case DEVICE_INFO:
      FetchDeviceInfo(&ola_client, opts);
      break;
    case DEVICE_PATCH:
      Patch(&ola_client, opts);
      break;
    case PLUGIN_INFO:
      FetchPluginInfo(&ola_client, opts);
      break;
    case PLUGIN_STATE:
      FetchPluginState(&ola_client, opts);
      break;
    case UNIVERSE_INFO:
      ola_client.GetClient()->FetchUniverseList(
          NewSingleCallback(&DisplayUniverses,
          ola_client.GetSelectServer(), opts.list_universe_ids));
      break;
    case UNIVERSE_NAME:
      SetUniverseName(&ola_client, opts);
      break;
    case UNI_MERGE:
      SetUniverseMergeMode(&ola_client, opts);
      break;
    case SET_DMX:
      SendDmx(&ola_client, opts);
      break;
    case SET_PORT_PRIORITY:
      SetPortPriority(&ola_client, opts);
  }

  ola_client.GetSelectServer()->Run();
  return 0;
}
