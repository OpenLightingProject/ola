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
 *  ola-client.cpp
 *  The multi purpose ola client.
 *  Copyright (C) 2005-2008 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/OlaClient.h>
#include <ola/SimpleClient.h>
#include <ola/network/SelectServer.h>
#include <olad/PortConstants.h>


#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using std::cout;
using std::endl;
using std::setw;
using std::string;
using std::vector;
using ola::OlaPlugin;
using ola::OlaUniverse;
using ola::OlaDevice;
using ola::OlaInputPort;
using ola::OlaOutputPort;
using ola::SimpleClient;
using ola::OlaClient;
using ola::network::SelectServer;

static const int INVALID_VALUE = -1;

/*
 * The mode is determined by the name in which we were called
 */
typedef enum {
  DEVICE_INFO,
  DEVICE_PATCH,
  PLUGIN_INFO,
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
  ola::PortDirection port_direction;  // input or output
  ola::PatchAction patch_action;      // patch or unpatch
  OlaUniverse::merge_mode merge_mode;  // the merge mode
  string cmd;      // argv[0]
  string uni_name;  // universe name
  string dmx;      // dmx string
  ola::port_priority_mode priority_mode;  // port priority mode
  uint8_t priority_value;  // port priority value
} options;


/*
 * The observer class which repsonds to events
 */
class Observer: public ola::OlaClientObserver {
  public:
    Observer(options *opts, SelectServer *ss): m_opts(opts), m_ss(ss) {}

    void Plugins(const vector <OlaPlugin> &plugins, const string &error);
    void PluginDescription(ola::ola_plugin_id plugin_id,
                           const string &description,
                           const string &error);
    void Devices(const vector <OlaDevice> &devices, const string &error);
    void Universes(const vector <OlaUniverse> &universes, const string &error);
    void PatchComplete(unsigned int device_alias,
                       unsigned int port,
                       ola::PortDirection port_direction,
                       const string &error);
    void UniverseNameComplete(unsigned int universe,
                              const string &error);
    void UniverseMergeModeComplete(unsigned int universe,
                                   const string &error);
    void SendDmxComplete(unsigned int universe,
                         const string &error);
    void SetPortPriorityComplete(unsigned int device_alias,
                                 unsigned int port,
                                 ola::PortDirection port_direction,
                                 const string &error);

  private:
    options *m_opts;
    SelectServer *m_ss;

    template<class PortClass>
    void ListPorts(const vector<PortClass> &ports, bool input);
};


/*
 * This is called when we recieve universe results from the client
 * @param universes a vector of OlaUniverses
 */
void Observer::Universes(const vector <OlaUniverse> &universes,
                         const string &error) {
  vector<OlaUniverse>::const_iterator iter;

  if (!error.empty()) {
    cout << error << endl;
    m_ss->Terminate();
    return;
  }

  cout << setw(5) << "Id" << "\t" << setw(30) << "Name" << "\t\tMerge Mode" <<
    endl;
  cout << "----------------------------------------------------------" << endl;

  for (iter = universes.begin(); iter != universes.end(); ++iter) {
    cout << setw(5) << iter->Id() << "\t" << setw(30) << iter->Name() << "\t\t"
      << (iter->MergeMode() == OlaUniverse::MERGE_HTP ? "HTP" : "LTP") << endl;
  }
  cout << "----------------------------------------------------------" << endl;
  m_ss->Terminate();
}


/*
 * @params plugins a vector of OlaPlugins
 */
void Observer::Plugins(const vector <OlaPlugin> &plugins,
                       const string &error) {
  vector<OlaPlugin>::const_iterator iter;

  if (!error.empty()) {
    cout << error << endl;
    m_ss->Terminate();
    return;
  }

  cout << setw(5) << "Id" << "\tPlugin Name" << endl;
  cout << "--------------------------------------" << endl;
  for (iter = plugins.begin(); iter != plugins.end(); ++iter)
    cout << setw(5) << iter->Id() << "\t" << iter->Name() << endl;
  cout << "--------------------------------------" << endl;
  m_ss->Terminate();
}


/*
 * Print a plugin description
 */
void Observer::PluginDescription(ola::ola_plugin_id plugin_id,
                                 const string &description,
                                 const string &error) {
  if (!error.empty())
    cout << error << endl;
  else
    cout << description << endl;
  m_ss->Terminate();
  return;
}


/*
 * @param devices a vector of OlaDevices
 */
void Observer::Devices(const vector <OlaDevice> &devices, const string &error) {
  printf("in devices\n");
  vector<OlaDevice>::const_iterator iter;

  if (!error.empty()) {
    cout << error << endl;
    m_ss->Terminate();
    return;
  }

  for (iter = devices.begin(); iter != devices.end(); ++iter) {
    cout << "Device " << iter->Alias() << ": " << iter->Name() << endl;
    vector<OlaInputPort> input_ports = iter->InputPorts();
    ListPorts(input_ports, true);
    vector<OlaOutputPort> output_ports = iter->OutputPorts();
    ListPorts(output_ports, false);
  }
  m_ss->Terminate();
}


/*
 * Called when the patch command completes.
 */
void Observer::PatchComplete(unsigned int device_alias,
                             unsigned int port,
                             ola::PortDirection port_direction,
                             const string &error) {
  if (!error.empty())
    cout << error << endl;
  m_ss->Terminate();
}

/*
 * Called when the name command completes.
 */
void Observer::UniverseNameComplete(unsigned int universe,
                                    const string &error) {
  if (!error.empty())
    cout << error << endl;
  m_ss->Terminate();
}


void Observer::UniverseMergeModeComplete(unsigned int universe,
                                         const string &error) {
  if (!error.empty())
    cout << error << endl;
  m_ss->Terminate();
}


void Observer::SendDmxComplete(unsigned int universe,
                               const string &error) {
  if (!error.empty())
    cout << error << endl;
  m_ss->Terminate();
}

void Observer::SetPortPriorityComplete(unsigned int device_alias,
                                       unsigned int port,
                                       ola::PortDirection port_direction,
                                       const string &error) {
  if (!error.empty())
    cout << error << endl;
  m_ss->Terminate();
}


template<class PortClass>
void Observer::ListPorts(const vector<PortClass> &ports, bool input) {
  typename vector<PortClass>::const_iterator port_iter;
  for (port_iter = ports.begin(); port_iter != ports.end(); ++port_iter) {
    cout << "  port " << port_iter->Id() << ", ";

    if (input)
      cout << "IN";
    else
      cout << "OUT";

    cout << " " << port_iter->Description();

    switch (port_iter->PriorityCapability()) {
      case ola::CAPABILITY_STATIC:
        cout << ", priority " << static_cast<int>(port_iter->Priority());
        break;
      case ola::CAPABILITY_FULL:
        cout << ", priority ";
        if (port_iter->PriorityMode() == ola::PRIORITY_MODE_INHERIT)
          cout << "inherited";
        else
          cout << "overide " << static_cast<int>(port_iter->Priority());
        break;
      default:
        break;
    }

    if (port_iter->IsActive())
      cout << ", patched to universe " << port_iter->Universe();
    cout << endl;
  }
}


/*
 * Init options
 */
void InitOptions(options *opts) {
  opts->m = DEVICE_INFO;
  opts->uni = INVALID_VALUE;
  opts->plugin_id = ola::OLA_PLUGIN_ALL;
  opts->help = false;
  opts->patch_action = ola::PATCH;
  opts->port_id = INVALID_VALUE;
  opts->port_direction = ola::OUTPUT_PORT;
  opts->device_id = INVALID_VALUE;
  opts->merge_mode = OlaUniverse::MERGE_HTP;
  opts->priority_mode = ola::PRIORITY_MODE_INHERIT;
  opts->priority_value = 0;
}


/*
 * Decide what mode we're running in
 */
void SetMode(options *opts) {
  string::size_type pos = opts->cmd.find_last_of("/");

  if (pos != string::npos)
    opts->cmd = opts->cmd.substr(pos + 1);

  if (opts->cmd == "ola_plugin_info")
    opts->m = PLUGIN_INFO;
  else if (opts->cmd == "ola_patch")
    opts->m = DEVICE_PATCH;
  else if (opts->cmd == "ola_uni_info")
    opts->m = UNIVERSE_INFO;
  else if (opts->cmd == "ola_uni_name")
    opts->m = UNIVERSE_NAME;
  else if (opts->cmd == "ola_uni_merge")
    opts->m = UNI_MERGE;
  else if (opts->cmd == "ola_set_dmx")
    opts->m = SET_DMX;
  else if (opts->cmd == "ola_set_priority")
    opts->m = SET_PORT_PRIORITY;
}


/*
 * parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"dmx", required_argument, 0, 'd'},
      {"help", no_argument, 0, 'h'},
      {"ltp", no_argument, 0, 'l'},
      {"name", required_argument, 0, 'n'},
      {"plugin_id", required_argument, 0, 'p'},
      {"universe", required_argument, 0, 'u'},
      {0, 0, 0, 0}
    };

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "ld:n:u:p:hv", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'd':
        opts->dmx = optarg;
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
      case 'u':
        opts->uni = atoi(optarg);
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

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'a':
        opts->patch_action = ola::PATCH;
        break;
      case 'd':
        opts->device_id = atoi(optarg);
        break;
      case 'p':
        opts->port_id = atoi(optarg);
        break;
      case 'r':
        opts->patch_action = ola::UNPATCH;
        break;
      case 'u':
        opts->uni = atoi(optarg);
        break;
      case 'h':
        opts->help = true;
        break;
      case 'i':
        opts->port_direction = ola::INPUT_PORT;
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

    if (c == -1)
      break;

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
        opts->port_direction = ola::INPUT_PORT;
        break;
      case 'o':
        opts->priority_mode = ola::PRIORITY_MODE_OVERRIDE;
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
  cout << "Usage: " << opts.cmd << " [--plugin_id <plugin_id>]\n"
  "\n"
  "Show information on the devices loaded by olad.\n"
  "\n"
  "  -h, --help                  Display this help message and exit.\n"
  "  -p, --plugin_id <plugin_id> Show only devices owned by this plugin.\n"
  << endl;
}


/*
 * Display the Patch help
 */
void DisplayPatchHelp(const options &opts) {
  cout << "Usage: " << opts.cmd <<
  " [--patch | --unpatch] --device <dev> --port <port> [--universe <uni>]\n"
  "\n"
  "Control ola port <-> universe mappings.\n"
  "\n"
  "  -a, --patch              Patch this port (default).\n"
  "  -d, --device <device>    Id of device to patch.\n"
  "  -h, --help               Display this help message and exit.\n"
  "  -p, --port <port>        Id of the port to patch.\n"
  "  -r, --unpatch            Unpatch this port.\n"
  "  -i, --input              Patch the input port (default is output).\n"
  "  -u, --universe <uni>     Id of the universe to patch to (default 0).\n"
  << endl;
}


/*
 * help message for plugin info
 */
void DisplayPluginInfoHelp(const options &opts) {
  cout << "Usage: " << opts.cmd <<
  " [--plugin_id <plugin_id>]\n"
  "\n"
  "Get info on the plugins loaded by olad. Called without arguments this will\n"
  "display the plugins loaded by olad. When used with --plugin_id this will \n"
  "display the specified plugin's description.\n"
  "\n"
  "  -h, --help                  Display this help message and exit.\n"
  "  -p, --plugin_id <plugin_id> Id of the plugin to fetch the description of\n"
  << endl;
}


/*
 * help message for uni info
 */
void DisplayUniverseInfoHelp(const options &opts) {
  cout << "Usage: " << opts.cmd <<
  "\n"
  "Shows info on the active universes in use.\n"
  "\n"
  "  -h, --help Display this help message and exit.\n"
  << endl;
}


/*
 * Help message for set uni name
 */
void DisplayUniverseNameHelp(const options &opts) {
  cout << "Usage: " << opts.cmd <<
  " --name <name> --universe <uni>\n"
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
  cout << "Usage: " << opts.cmd <<
  " --universe <uni> [ --ltp]\n"
  "\n"
  "Change the merge mode for the specified universe. Without --ltp it will\n"
  "revert to HTP mode.\n"
  "\n"
  "  -h, --help                Display this help message and exit.\n"
  "  -l, --ltp                 Change to ltp mode.\n"
  "  -u, --universe <universe> Id of the universe to change.\n"
  << endl;
}



/*
 * Help message for set dmx
 */
void DisplaySetDmxHelp(const options &opts) {
  cout << "Usage: " << opts.cmd <<
  " --universe <universe> --dmx 0,255,0,255\n"
  "\n"
  "Sets the DMX values for a universe.\n"
  "\n"
  "  -h, --help                Display this help message and exit.\n"
  "  -u, --universe <universe> Universe number.\n"
  "  -d, --dmx <values>        Comma separated DMX values.\n"
  << endl;
}

/*
 * Display the Patch help
 */
void DisplaySetPriorityHelp(const options &opts) {
  cout << "Usage: " << opts.cmd <<
  " --device <dev> --port <port> [--override <value>]\n"
  "\n"
  "Set a port's priority, without the --override flag this will set the port\n"
  "to inherit mode.\n"
  "\n"
  "  -d, --device <device>    Id of device to patch.\n"
  "  -h, --help               Display this help message and exit.\n"
  "  -i, --input              Set an input port\n"
  "  -o, --override <value>   Set the port priority to a static value.\n"
  "  -p, --port <port>        Id of the port to patch.\n"
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
int FetchDeviceInfo(OlaClient *client, const options &opts) {
  printf("calling fetch");
  client->FetchDeviceInfo((ola::ola_plugin_id) opts.plugin_id);
  return 0;
}


void Patch(OlaClient *client, const options &opts) {
  if (opts.device_id == INVALID_VALUE || opts.port_id == INVALID_VALUE) {
    DisplayPatchHelp(opts);
    exit(1);
  }

  if (opts.patch_action == ola::PATCH  && opts.uni == INVALID_VALUE) {
    DisplayPatchHelp(opts);
    exit(1);
  }
  client->Patch(opts.device_id,
                opts.port_id,
                opts.port_direction,
                opts.patch_action, opts.uni);
}


/*
 * Fetch information on plugins.
 */
int FetchPluginInfo(OlaClient *client, const options &opts) {
  if (opts.plugin_id > 0)
    client->FetchPluginDescription((ola::ola_plugin_id) opts.plugin_id);
  else
    client->FetchPluginList();
  return 0;
}



/*
 * send a set name request
 * @param client the ola client
 * @param opts  the const options
 */
int SetUniverseName(OlaClient *client, const options &opts) {
  if (opts.uni == INVALID_VALUE) {
    DisplayUniverseNameHelp(opts);
    exit(1);
  }
  client->SetUniverseName(opts.uni, opts.uni_name);
  return 0;
}


/*
 * send a set name request
 * @param client the ola client
 * @param opts  the const options
 */
int SetUniverseMergeMode(OlaClient *client, const options &opts) {
  if (opts.uni == INVALID_VALUE) {
    DisplayUniverseMergeHelp(opts);
    exit(1);
  }
  client->SetUniverseMergeMode(opts.uni, opts.merge_mode);
  return 0;
}


/*
 * Send a dmx message
 * @param client the ola client
 * @param opts the options
 */
int SendDmx(OlaClient *client, const options &opts) {
  ola::DmxBuffer buffer;
  bool status = buffer.SetFromString(opts.dmx);

  if (opts.uni < 0 || !status || buffer.Size() == 0) {
    DisplaySetDmxHelp(opts);
    exit(1);
  }

  if (!client->SendDmx(opts.uni, buffer)) {
    cout << "Send DMX failed" << endl;
    return 1;
  }
  return 0;
}


/*
 * Set the priority of a port
 */
void SetPortPriority(OlaClient *client, const options &opts) {
  if (opts.device_id == INVALID_VALUE || opts.port_id == INVALID_VALUE) {
    DisplaySetPriorityHelp(opts);
    exit(1);
  }

  if (opts.priority_mode == ola::PRIORITY_MODE_INHERIT) {
    client->SetPortPriorityInherit(opts.device_id,
                                   opts.port_id,
                                   opts.port_direction);
  } else if (opts.priority_mode == ola::PRIORITY_MODE_OVERRIDE) {
    client->SetPortPriorityOverride(opts.device_id,
                                    opts.port_id,
                                    opts.port_direction,
                                    opts.priority_value);
  } else {
    DisplaySetPriorityHelp(opts);
  }
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);
  SimpleClient ola_client;
  options opts;

  InitOptions(&opts);
  opts.cmd = argv[0];

  // decide how we should behave
  SetMode(&opts);

  if (opts.m == DEVICE_PATCH)
    ParsePatchOptions(argc, argv, &opts);
  else if (opts.m == SET_PORT_PRIORITY)
    ParseSetPriorityOptions(argc, argv, &opts);
  else
    ParseOptions(argc, argv, &opts);

  if (opts.help)
    DisplayHelpAndExit(opts);

  if (!ola_client.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(1);
  }

  OlaClient *client = ola_client.GetClient();
  SelectServer *ss = ola_client.GetSelectServer();

  Observer observer(&opts, ss);
  client->SetObserver(&observer);

  switch (opts.m) {
    case DEVICE_INFO:
      FetchDeviceInfo(client, opts);
      break;
    case DEVICE_PATCH:
      Patch(client, opts);
      break;
    case PLUGIN_INFO:
      FetchPluginInfo(client, opts);
      break;
    case UNIVERSE_INFO:
      client->FetchUniverseInfo();
      break;
    case UNIVERSE_NAME:
      SetUniverseName(client, opts);
      break;
    case UNI_MERGE:
      SetUniverseMergeMode(client, opts);
      break;
    case SET_DMX:
      SendDmx(client, opts);
      break;
    case SET_PORT_PRIORITY:
      SetPortPriority(client, opts);
  }

  ss->Run();
  return 0;
}
