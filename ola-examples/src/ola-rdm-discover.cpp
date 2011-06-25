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
 *  ola-discover.cpp
 *  Print the list of UIDs and force RDM discovery
 *  Copyright (C) 2010 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <ola/Logging.h>
#include <ola/OlaCallbackClient.h>
#include <ola/OlaClientWrapper.h>
#include <ola/network/SelectServer.h>


#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using std::cout;
using std::cerr;
using std::endl;
using std::setw;
using std::string;
using std::vector;
using ola::OlaCallbackClient;
using ola::OlaCallbackClientWrapper;
using ola::network::SelectServer;
using ola::rdm::UIDSet;

static const int INVALID_VALUE = -1;

typedef struct {
  int uni;         // universe id
  bool help;       // show the help
  bool full;       // full discovery
  bool incremental; // incremental discovery
  string cmd;      // argv[0]
} options;


SelectServer *ss;

/*
 * This is called when we recieve uids for a universe
 * @param universes a vector of OlaUniverses
 */
void UIDList(const ola::rdm::UIDSet &uids,
                       const string &error) {
  if (error.empty()) {
    UIDSet::Iterator iter = uids.Begin();
    for (; iter != uids.End(); ++iter) {
      cout << *iter << endl;
    }
  } else {
    cout << error << endl;
  }
  ss->Terminate();
}


/*
 * Called once we get an ack for the discovery request
 */
void ForceRDMDiscoveryComplete(const string &error) {
  if (!error.empty())
    cout << error << endl;
  ss->Terminate();
}


/*
 * parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  opts->uni = INVALID_VALUE;
  opts->help = false;
  opts->full = false;
  opts->incremental = false;

  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"full", no_argument, 0, 'f'},
      {"incremental", no_argument, 0, 'i'},
      {"universe", required_argument, 0, 'u'},
      {0, 0, 0, 0}
    };

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "u:fhi", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'f':
        opts->full = true;
        break;
      case 'h':
        opts->help = true;
        break;
      case 'i':
        opts->incremental = true;
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
  " --universe <universe> [--force_discovery]\n"
  "\n"
  "Fetch the UID list for a universe.\n"
  "\n"
  "  -h, --help        Display this help message and exit.\n"
  "  -f, --full        Force full RDM Discovery for this universe\n"
  "  -i, --incremental Force incremental RDM Discovery for this universe\n"
  "  -u, --universe <universe> Universe number.\n"
  << endl;
}


/*
 * Send a fetch uid list request
 * @param client  the ola client
 * @param opts  the const options
 */
bool FetchUIDs(OlaCallbackClient *client, const options &opts) {
  if (opts.uni == INVALID_VALUE) {
    DisplayGetUIDsHelp(opts);
    return false;
  }

  if (opts.full) {
    return client->RunDiscovery(
        opts.uni,
        true,
        ola::NewSingleCallback(&ForceRDMDiscoveryComplete));
  } else if (opts.incremental) {
    return client->RunDiscovery(
        opts.uni,
        false,
        ola::NewSingleCallback(&ForceRDMDiscoveryComplete));
  } else {
    return client->FetchUIDList(
        opts.uni,
        ola::NewSingleCallback(&UIDList));
  }
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);
  OlaCallbackClientWrapper ola_client;
  options opts;
  opts.cmd = argv[0];

  ParseOptions(argc, argv, &opts);

  if (opts.help)
    DisplayGetUIDsHelp(opts);

  if (opts.full && opts.incremental) {
    cerr << "Only one of -i and -f can be specified" << endl;
    exit(1);
  }

  if (!ola_client.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(1);
  }

  OlaCallbackClient *client = ola_client.GetClient();
  ss = ola_client.GetSelectServer();

  if (FetchUIDs(client, opts))
    ss->Run();
  return 0;
}
