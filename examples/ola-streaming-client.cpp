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
 *  ola-client.cpp
 *  The multi purpose ola client.
 *  Copyright (C) 2005-2008 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/StreamingClient.h>
#include <ola/StringUtils.h>

#include <iostream>
#include <string>

using std::cout;
using std::endl;
using std::string;
using ola::StreamingClient;


typedef struct {
  string dmx_data;
  unsigned int universe;
  bool help;
} options;

bool terminate = false;


/*
 * parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"dmx", required_argument, 0, 'd'},
      {"help", no_argument, 0, 'h'},
      {"universe", required_argument, 0, 'u'},
      {0, 0, 0, 0}
    };

  opts->dmx_data = "";
  opts->universe = 1;
  opts->help = false;

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "d:hu:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'd':
        opts->dmx_data = optarg;
        break;
      case 'h':
        opts->help = true;
        break;
      case 'u':
        opts->universe = atoi(optarg);
        break;
      case '?':
        break;
      default:
        break;
    }
  }
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char arg[]) {
  cout << "Usage: " << arg <<
  " --dmx <dmx_data> --universe <universe_id>\n"
  "\n"
  "Send DMX512 data to OLA. If dmx data isn't provided we read from stdin.\n"
  "\n"
  "  -d, --dmx <dmx_data>         DMX512 data, e.g. '1,240,0,255'\n"
  "  -h, --help                   Display this help message and exit.\n"
  "  -u, --universe <universe_id> Id of universe to send data for.\n"
  << endl;
  exit(1);
}


bool SendDataFromString(StreamingClient &client,
                        unsigned int universe,
                        const string &data) {
  ola::DmxBuffer buffer;
  bool status = buffer.SetFromString(data);

  if (!status || buffer.Size() == 0)
    return false;

  if (!client.SendDmx(universe, buffer)) {
    cout << "Send DMX failed" << endl;
    terminate = true;
    return false;
  }
  return true;
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);
  StreamingClient ola_client;
  options opts;

  ParseOptions(argc, argv, &opts);

  if (opts.help)
    DisplayHelpAndExit(argv[0]);

  if (!ola_client.Setup()) {
    OLA_FATAL << "Setup failed";
    exit(1);
  }

  if (opts.dmx_data.empty()) {
    string input;
    while (!terminate && std::cin >> input) {
      ola::StringTrim(&input);
      SendDataFromString(ola_client, opts.universe, input);
    }
  } else {
    SendDataFromString(ola_client, opts.universe, opts.dmx_data);
  }
  ola_client.Stop();
  return 0;
}
