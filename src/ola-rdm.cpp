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
#include <ola/Logging.h>
#include <ola/OlaClient.h>
#include <ola/SimpleClient.h>
#include <ola/network/SelectServer.h>


#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using std::cout;
using std::endl;
using std::setw;
using std::string;
using std::vector;
using ola::rdm::UIDSet;
using ola::SimpleClient;
using ola::OlaClient;
using ola::network::SelectServer;

static const int INVALID_VALUE = -1;

/*
 * The mode is determined by the name in which we were called
 */
typedef enum {
  GET_UIDS,
  GET_PID,
  SET_PID,
} mode;


typedef struct {
  mode m;          // mode
  int uni;         // universe id
  bool help;       // show the help
  string cmd;      // argv[0]
} options;


/*
 * The observer class which repsonds to events
 */
class Observer: public ola::OlaClientObserver {
  public:
    Observer(options *opts, SelectServer *ss): m_opts(opts), m_ss(ss) {}

    void UIDList(unsigned int universe,
                 const ola::rdm::UIDSet &uids,
                 const string &error);

  private:
    options *m_opts;
    SelectServer *m_ss;
};


/*
 * This is called when we recieve uids for a universe
 * @param universes a vector of OlaUniverses
 */
void Observer::UIDList(unsigned int universe,
                       const ola::rdm::UIDSet &uids,
                       const string &error) {
  if (error.empty()) {
    UIDSet::Iterator iter = uids.Begin();
    for (; iter != uids.End(); ++iter) {
      cout << *iter << endl;
    }
  } else {
    cout << error << endl;
  }
  m_ss->Terminate();
}


/*
 * Init options
 */
void InitOptions(options *opts) {
  opts->m = GET_UIDS;
  opts->uni = INVALID_VALUE;
  opts->help = false;
}


/*
 * Decide what mode we're running in
 */
void SetMode(options *opts) {
  string::size_type pos = opts->cmd.find_last_of("/");

  if (pos != string::npos)
    opts->cmd = opts->cmd.substr(pos + 1);

  if (opts->cmd == "ola_get_pid")
    opts->m = GET_PID;
  else if (opts->cmd == "ola_set_pid")
    opts->m = SET_PID;
}


/*
 * parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"universe", required_argument, 0, 'u'},
      {0, 0, 0, 0}
    };

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "u:h", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'h':
        opts->help = true;
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
 * Help message for set dmx
 */
void DisplayGetUIDsHelp(const options &opts) {
  cout << "Usage: " << opts.cmd <<
  " --universe <universe>\n"
  "\n"
  "Fetch the UID list for a universe.\n"
  "\n"
  "  -h, --help                Display this help message and exit.\n"
  "  -u, --universe <universe> Universe number.\n"
  << endl;
}


/*
 * Display the help for get_pid
 */
void DisplayGetPidHelp(const options &opts) {
  cout << "usage: " << opts.cmd <<
  " --universe <universe> --uid <uid> -p <pid>\n"
  "\n"
  "Get the value of a pid for a device.\n"
  "\n"
  "  -h, --help               display this help message and exit.\n"
  "  -u, --universe <universe> universe number.\n"
  << endl;
}


/*
 * Display the help for set_pid
 */
void DisplaySetPidHelp(const options &opts) {
  cout << "usage: " << opts.cmd <<
  " --universe <universe> --uid <uid> -p <pid>\n"
  "\n"
  "Set the value of a pid for a device.\n"
  "\n"
  "  -h, --help               display this help message and exit.\n"
  "  -u, --universe <universe> universe number.\n"
  << endl;
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(const options &opts) {
  switch (opts.m) {
    case GET_UIDS:
      DisplayGetUIDsHelp(opts);
      break;
    case GET_PID:
      DisplayGetPidHelp(opts);
      break;
    case SET_PID:
      DisplaySetPidHelp(opts);
      break;
  }
  exit(0);
}


/*
 * Send a fetch uid list request
 * @param client  the ola client
 * @param opts  the const options
 */
bool FetchUIDs(OlaClient *client, const options &opts) {
  if (opts.uni == INVALID_VALUE) {
    DisplayHelpAndExit(opts);
    return false;
  }

  return client->FetchUIDList(opts.uni);
}

/*
 * Send a fetch uid list request
 * @param client  the ola client
 * @param opts  the const options
 */
bool GetPID(OlaClient *client, const options &opts) {
  // client->FetchUIDList(opts.uni);
  return false;
}

/*
 * Send a fetch uid list request
 * @param client  the ola client
 * @param opts  the const options
 */
bool SetPID(OlaClient *client, const options &opts) {
  // client->FetchUIDList(opts.uni);
  return false;
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

  bool ret = false;
  switch (opts.m) {
    case GET_UIDS:
      ret = FetchUIDs(client, opts);
      break;
    case GET_PID:
      ret = GetPID(client, opts);
      break;
    case SET_PID:
      ret = SetPID(client, opts);
      break;
  }

  if (ret)
    ss->Run();
  return 0;
}
