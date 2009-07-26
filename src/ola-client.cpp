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

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include <ola/SimpleClient.h>
#include <ola/OlaClient.h>
#include <ola/DmxBuffer.h>
#include <ola/network/SelectServer.h>

using ola::OlaPlugin;
using ola::OlaUniverse;
using ola::OlaDevice;
using ola::OlaPort;
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
} mode;


typedef struct {
  mode m;          // mode
  int uni;         // universe id
  unsigned int plugin_id;   // plugin id
  bool help;       // show the help
  int device_id;   // device id
  int port_id;     // port id
  ola::PatchAction patch_action;      // patch or unpatch
  OlaUniverse::merge_mode merge_mode; // the merge mode
  string cmd;      // argv[0]
  string uni_name; // universe name
  string dmx;      // dmx string
} options;


/*
 * The observer class which repsonds to events
 */
class Observer: public ola::OlaClientObserver {
  public:
    Observer(options *opts, SelectServer *ss): m_opts(opts), m_ss(ss) {}

    void Plugins(const vector <OlaPlugin> &plugins, const string &error);
    void Devices(const vector <OlaDevice> devices, const string &error);
    void Universes(const vector <OlaUniverse> universes, const string &error);
    void PatchComplete(const string &error);
    void UniverseNameComplete(const string &error);
    void UniverseMergeModeComplete(const string &error);
    void SendDmxComplete(const string &error);

  private:
    options *m_opts;
    SelectServer *m_ss;
};


/*
 * This is called when we recieve universe results from the client
 * @param universes a vector of OlaUniverses
 */
void Observer::Universes(const vector <OlaUniverse> universes,
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

  for(iter = universes.begin(); iter != universes.end(); ++iter) {
    cout << setw(5) << iter->Id() << "\t" << setw(30) << iter->Name() << "\t\t"
      << (iter->MergeMode() == OlaUniverse::MERGE_HTP ? "HTP" : "LTP") << endl;
  }
  cout << "----------------------------------------------------------" << endl;
  m_ss->Terminate();
}


/*
 * @params plugins a vector of OlaPlugins
 */
void Observer::Plugins(const vector <OlaPlugin> &plugins, const string &error) {
  vector<OlaPlugin>::const_iterator iter;

  if (!error.empty()) {
    cout << error << endl;
    m_ss->Terminate();
    return;
  }

  if (m_opts->plugin_id > 0 && m_opts->plugin_id < OLA_PLUGIN_LAST) {
    for(iter = plugins.begin(); iter != plugins.end(); ++iter) {
      if(iter->Id() == m_opts->plugin_id)
        cout << iter->Description() << endl;
    }
  } else {
    cout << setw(5) << "Id" << "\tDevice Name" << endl;
    cout << "--------------------------------------" << endl;

    for(iter = plugins.begin(); iter != plugins.end(); ++iter)
      cout << setw(5) << iter->Id() << "\t" << iter->Name() << endl;
    cout << "--------------------------------------" << endl;
  }
  m_ss->Terminate();
}


/*
 * @param devices a vector of OlaDevices
 */
void Observer::Devices(const vector <OlaDevice> devices, const string &error) {
  vector<OlaDevice>::const_iterator iter;

  if (!error.empty()) {
    cout << error << endl;
    m_ss->Terminate();
    return;
  }

  for (iter = devices.begin(); iter != devices.end(); ++iter) {
    cout << "Device " << iter->Alias() << ": " << iter->Name() << endl;
    vector<OlaPort> ports = iter->Ports();
    vector<OlaPort>::const_iterator port_iter;

    for (port_iter = ports.begin(); port_iter != ports.end(); ++port_iter) {
      cout << "  port " << port_iter->Id() << ", ";

      if (port_iter->Capability() == OlaPort::OLA_PORT_CAP_IN)
        cout << "IN";
      else
        cout << "OUT";

      cout << " " << port_iter->Description();

      if (port_iter->IsActive())
        cout << ", OLA universe " << port_iter->Universe();
      cout << endl;
    }
  }
  m_ss->Terminate();
}


/*
 * Called when the patch command completes.
 */
void Observer::PatchComplete(const string &error) {
  if (!error.empty())
    cout << error << endl;
  m_ss->Terminate();
}

/*
 * Called when the name command completes.
 */
void Observer::UniverseNameComplete(const string &error) {
  if (!error.empty())
    cout << error << endl;
  m_ss->Terminate();
}


void Observer::UniverseMergeModeComplete(const string &error) {
  if (!error.empty())
    cout << error << endl;
  m_ss->Terminate();
}


void Observer::SendDmxComplete(const string &error) {
  if (!error.empty())
    cout << error << endl;
  m_ss->Terminate();
}


/*
 * Init options
 */
void InitOptions(options &opts) {
  opts.m = DEVICE_INFO;
  opts.uni = INVALID_VALUE;
  opts.plugin_id = INVALID_VALUE;
  opts.help = false;
  opts.patch_action = ola::PATCH;
  opts.port_id = INVALID_VALUE;
  opts.device_id = INVALID_VALUE;
  opts.merge_mode = OlaUniverse::MERGE_HTP;
}


/*
 * Decide what mode we're running in
 */
void SetMode(options &opts) {
  string::size_type pos = opts.cmd.find_last_of("/");

  if (pos != string::npos)
    opts.cmd = opts.cmd.substr(pos + 1);

  if (opts.cmd == "ola_plugin_info")
    opts.m = PLUGIN_INFO;
  else if (opts.cmd == "ola_patch")
    opts.m = DEVICE_PATCH;
  else if (opts.cmd == "ola_uni_info")
    opts.m = UNIVERSE_INFO;
  else if (opts.cmd == "ola_uni_name")
    opts.m = UNIVERSE_NAME;
  else if (opts.cmd == "ola_uni_merge")
    opts.m = UNI_MERGE;
  else if (opts.cmd == "ola_set_dmx")
    opts.m = SET_DMX;
}


/*
 * parse our cmd line options
 *
 */
void ParseOptions(int argc, char *argv[], options &opts) {
  static struct option long_options[] = {
      {"plugin_id", required_argument, 0, 'p'},
      {"help", no_argument, 0, 'h'},
      {"ltp", no_argument, 0, 'l'},
      {"name", required_argument, 0, 'n'},
      {"universe", required_argument, 0, 'u'},
      {"dmx", required_argument, 0, 'd'},
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
      case 'p':
        opts.plugin_id = atoi(optarg);
        break;
      case 'h':
        opts.help = true;
        break;
      case 'l':
        opts.merge_mode = OlaUniverse::MERGE_LTP;
        break;
      case 'n':
        opts.uni_name = optarg;
        break;
      case 'u':
        opts.uni = atoi(optarg);
        break;
      case 'd':
        opts.dmx = optarg;
        break;
      case '?':
        break;
      default:
        ;
    }
  }
}


/*
 * parse our cmd line options for the patch command
 */
int ParsePatchOptions(int argc, char *argv[], options &opts) {
  static struct option long_options[] = {
      {"patch", no_argument, 0, 'a'},
      {"unpatch", no_argument, 0, 'r'},
      {"device", required_argument, 0, 'd'},
      {"port", required_argument, 0, 'p'},
      {"universe", required_argument, 0, 'u'},
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0}
    };

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "ard:p:u:h", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'a':
        opts.patch_action = ola::PATCH;
        break;
      case 'd':
        opts.device_id = atoi(optarg);
        break;
      case 'p':
        opts.port_id = atoi(optarg);
        break;
      case 'r':
        opts.patch_action = ola::UNPATCH;
        break;
      case 'u':
        opts.uni = atoi(optarg);
        break;
      case 'h':
        opts.help = true;
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
  "display the plugins loaded by olad. When used with --plugin_id this wilk display\n"
  "the specified plugin's description\n"
  "\n"
  "  -h, --help                  Display this help message and exit.\n"
  "  -p, --plugin_id <plugin_id> Id of the plugin to fetch the description of.\n"
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
void display_set_dmx_help(const options &opts) {
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
      display_set_dmx_help(opts);
      break;
  }
  exit(0);
}


/*
 * Send a fetch device info request
 * @param client  the ola client
 * @param opts  the const options
 */
int FetchDeviceInfo(OlaClient *client, const options &opts) {
  if (opts.plugin_id > 0 && opts.plugin_id < OLA_PLUGIN_LAST)
    client->FetchDeviceInfo((ola_plugin_id) opts.plugin_id);
  else
    client->FetchDeviceInfo();
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
  client->Patch(opts.device_id, opts.port_id, opts.patch_action, opts.uni);
}


/*
 * Fetch information on plugins.
 */
int FetchPluginInfo(OlaClient *client, const options &opts) {
  if (opts.plugin_id > 0 && opts.plugin_id < OLA_PLUGIN_LAST)
    client->FetchPluginInfo((ola_plugin_id) opts.plugin_id, true);
  else
    client->FetchPluginInfo();
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
    exit(1) ;
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
    exit(1) ;
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
    display_set_dmx_help(opts) ;
    exit(1);
  }

  if (!client->SendDmx(opts.uni, buffer)) {
    cout << "Send DMX failed" << endl;
    return 1;
  }
  return 0;
}


/*
 *
 */
int main(int argc, char *argv[]) {
  SimpleClient ola_client;
  options opts;

  InitOptions(opts);
  opts.cmd = argv[0];

  // decide how we should behave
  SetMode(opts);

  if (opts.m == DEVICE_PATCH)
    ParsePatchOptions(argc, argv, opts);
  else
    ParseOptions(argc, argv, opts);

  if (opts.help)
    DisplayHelpAndExit(opts);

  if (!ola_client.Setup()) {
    cout << "error: " << strerror(errno) << endl;
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
  }

  ss->Run();
  return 0;
}
