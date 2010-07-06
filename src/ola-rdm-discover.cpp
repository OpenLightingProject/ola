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


typedef struct {
  int uni;         // universe id
  bool help;       // show the help
  bool force_discovery;  // force RDM discovery
  string cmd;      // argv[0]
} options;


/*
 * The observer class which responds to events
 */
class Observer: public ola::OlaClientObserver {
  public:
    Observer(options *opts, SelectServer *ss): m_opts(opts), m_ss(ss) {}

    void UIDList(unsigned int universe,
                 const ola::rdm::UIDSet &uids,
                 const string &error);
    void ForceRDMDiscoveryComplete(unsigned int universe,
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
 * Called once we get an ack for the discovery request
 */
void Observer::ForceRDMDiscoveryComplete(unsigned int universe,
                                         const string &error) {
  if (!error.empty())
    cout << error << endl;
  m_ss->Terminate();
}


/*
 * parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  opts->uni = INVALID_VALUE;
  opts->help = false;
  opts->force_discovery = false;

  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"force_discovery", no_argument, 0, 'f'},
      {"universe", required_argument, 0, 'u'},
      {0, 0, 0, 0}
    };

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "u:hf", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'f':
        opts->force_discovery = true;
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
  " --universe <universe> [--force_discovery]\n"
  "\n"
  "Fetch the UID list for a universe.\n"
  "\n"
  "  -h, --help                Display this help message and exit.\n"
  "  -f, --force_discovery     Force RDM Discovery for this universe\n"
  "  -u, --universe <universe> Universe number.\n"
  << endl;
}


/*
 * Send a fetch uid list request
 * @param client  the ola client
 * @param opts  the const options
 */
bool FetchUIDs(OlaClient *client, const options &opts) {
  if (opts.uni == INVALID_VALUE) {
    DisplayGetUIDsHelp(opts);
    return false;
  }

  if (opts.force_discovery)
    return client->ForceDiscovery(opts.uni);
  else
    return client->FetchUIDList(opts.uni);
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);
  SimpleClient ola_client;
  options opts;
  opts.cmd = argv[0];

  ParseOptions(argc, argv, &opts);

  if (opts.help)
    DisplayGetUIDsHelp(opts);

  if (!ola_client.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(1);
  }

  OlaClient *client = ola_client.GetClient();
  SelectServer *ss = ola_client.GetSelectServer();

  Observer observer(&opts, ss);
  client->SetObserver(&observer);

  if (FetchUIDs(client, opts))
    ss->Run();
  return 0;
}
