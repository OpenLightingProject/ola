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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#include <ola/base/SysExits.h>
#include <ola/io/SelectServer.h>
#include <ola/rdm/UID.h>

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
using ola::io::SelectServer;
using ola::rdm::UID;
using ola::rdm::UIDSet;

static const int INVALID_VALUE = -1;

typedef struct {
  int uni;         // universe id
  bool help;       // show the help
  bool full;       // full discovery
  bool incremental;  // incremental discovery
  bool broadcast;  // broadcast UID
  bool vendorcast;  // vendorcast UID
  string cmd;      // argv[0]
} options;


options opts;
SelectServer *ss;

/*
 * This is called when we recieve uids for a universe
 * @param universes a vector of OlaUniverses
 */
void UIDList(const ola::rdm::UIDSet &uids,
             const string &error) {
  UIDSet vendorcast;
  if (error.empty()) {
    UIDSet::Iterator iter = uids.Begin();
    for (; iter != uids.End(); ++iter) {
      cout << *iter << endl;
      vendorcast.AddUID(iter->AllManufacturerDevices());
    }
    if (opts.vendorcast) {
      iter = vendorcast.Begin();
      for (; iter != vendorcast.End(); ++iter) {
        cout << *iter << endl;
      }
    }
    if (opts.broadcast) {
      cout << UID::AllDevices().ToString() << endl;
    }
  } else {
    cerr << error << endl;
  }
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
  opts->broadcast = false;
  opts->vendorcast = false;

  static struct option long_options[] = {
      {"include-broadcast", no_argument, 0, 'b'},
      {"help", no_argument, 0, 'h'},
      {"full", no_argument, 0, 'f'},
      {"incremental", no_argument, 0, 'i'},
      {"universe", required_argument, 0, 'u'},
      {"include-vendorcast", no_argument, 0, 'v'},
      {0, 0, 0, 0}
    };

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "u:bfhiv", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'b':
        opts->broadcast = true;
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
      case 'v':
        opts->vendorcast = true;
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
  " --universe <universe> [--full|--incremental]\n"
  "\n"
  "Fetch the UID list for a universe.\n"
  "\n"
  "  -b, --include-broadcast   Include broadcast UID for this universe\n"
  "  -h, --help                Display this help message and exit.\n"
  "  -f, --full                Force full RDM Discovery for this universe\n"
  "  -i, --incremental         Force incremental RDM Discovery for this universe\n"
  "  -u, --universe <universe> Universe number.\n"
  "  -v, --include-vendorcast  Include vendorcast UID for this universe\n"
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
        ola::NewSingleCallback(&UIDList));
  } else if (opts.incremental) {
    return client->RunDiscovery(
        opts.uni,
        false,
        ola::NewSingleCallback(&UIDList));
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
  opts.cmd = argv[0];

  ParseOptions(argc, argv, &opts);

  if (opts.help) {
    DisplayGetUIDsHelp(opts);
    exit(ola::EXIT_OK);
  }

  if (opts.full && opts.incremental) {
    cerr << "Only one of -i and -f can be specified" << endl;
    exit(ola::EXIT_USAGE);
  }

  if (!ola_client.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(ola::EXIT_UNAVAILABLE);
  }

  OlaCallbackClient *client = ola_client.GetClient();
  ss = ola_client.GetSelectServer();

  if (FetchUIDs(client, opts))
    ss->Run();
  return ola::EXIT_OK;
}
