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
 * ola-rdm-discover.cpp
 * Print the list of UIDs and force RDM discovery
 * Copyright (C) 2010 Simon Newton
 */

#include <errno.h>
#include <ola/Logging.h>
#include <ola/OlaCallbackClient.h>
#include <ola/OlaClientWrapper.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
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

DEFINE_s_uint32(universe, u, 1, "The universe to do RDM discovery on");
DEFINE_s_default_bool(full, f, false,
                      "Force full RDM Discovery for this universe");
DEFINE_s_default_bool(incremental, i, false,
                      "Force incremental RDM Discovery for this universe");
DEFINE_default_bool(include_broadcast, false,
                    "Include broadcast UID for this universe");
DEFINE_default_bool(include_vendorcast, false,
                    "Include vendorcast UID for this universe");

SelectServer *ss;

/*
 * This is called when we receive uids for a universe
 * @param universes a vector of OlaUniverses
 */
void UIDList(const ola::rdm::UIDSet &uids,
             const string &error) {
  UIDSet vendorcast;
  if (error.empty()) {
    UIDSet::Iterator iter = uids.Begin();
    for (; iter != uids.End(); ++iter) {
      cout << *iter << endl;
      vendorcast.AddUID(UID::VendorcastAddress(*iter));
    }
    if (FLAGS_include_vendorcast) {
      iter = vendorcast.Begin();
      for (; iter != vendorcast.End(); ++iter) {
        cout << *iter << endl;
      }
    }
    if (FLAGS_include_broadcast) {
      cout << UID::AllDevices().ToString() << endl;
    }
  } else {
    cerr << error << endl;
  }
  ss->Terminate();
}

/*
 * Send a fetch uid list request
 * @param client  the ola client
 */
bool FetchUIDs(OlaCallbackClient *client) {
  if (FLAGS_full) {
    return client->RunDiscovery(
        FLAGS_universe,
        true,
        ola::NewSingleCallback(&UIDList));
  } else if (FLAGS_incremental) {
    return client->RunDiscovery(
        FLAGS_universe,
        false,
        ola::NewSingleCallback(&UIDList));
  } else {
    return client->FetchUIDList(
        FLAGS_universe,
        ola::NewSingleCallback(&UIDList));
  }
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::AppInit(
      &argc,
      argv,
      "--universe <universe> [--full|--incremental]",
      "Fetch the UID list for a universe.");

  OlaCallbackClientWrapper ola_client;

  if (!FLAGS_universe.present()) {
    ola::DisplayUsageAndExit();
  }

  if (FLAGS_full && FLAGS_incremental) {
    cerr << "Only one of -i and -f can be specified" << endl;
    exit(ola::EXIT_USAGE);
  }

  if (!ola_client.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(ola::EXIT_UNAVAILABLE);
  }

  OlaCallbackClient *client = ola_client.GetClient();
  ss = ola_client.GetSelectServer();

  if (FetchUIDs(client)) {
    ss->Run();
  }
  return ola::EXIT_OK;
}
