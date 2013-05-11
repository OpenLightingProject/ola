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
 *  ola-throughput.cpp
 *  Send a bunch of frames quickly to load test the server.
 *  Copyright (C) 2005-2010 Simon Newton
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
  unsigned int universe;
  unsigned int sleep_time;
  bool help;
} options;


/*
 * parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"sleep", required_argument, 0, 's'},
      {"universe", required_argument, 0, 'u'},
      {0, 0, 0, 0}
    };

  opts->sleep_time = 40000;
  opts->universe = 1;
  opts->help = false;

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "hs:u:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'h':
        opts->help = true;
        break;
      case 's':
        opts->sleep_time = atoi(optarg);
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
  "  -h, --help                   Display this help message and exit.\n"
  "  -s, --sleep <time_in_uS>     Time to sleep between frames.\n"
  "  -u, --universe <universe_id> Id of universe to send data for.\n"
  << endl;
  exit(1);
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

  while (1) {
    usleep(opts.sleep_time);
    ola::DmxBuffer buffer;
    buffer.Blackout();

    if (!ola_client.SendDmx(opts.universe, buffer)) {
      cout << "Send DMX failed" << endl;
      return false;
    }
  }
  return 0;
}
