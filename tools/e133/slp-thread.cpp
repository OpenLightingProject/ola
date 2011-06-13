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
 * slp-thread.cpp
 * Copyright (C) 2011 Simon Newton
 *
 */

#include <getopt.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/network/SelectServer.h>
#include <signal.h>
#include <sysexits.h>

#include <iostream>
#include <string>
#include <vector>

#include "SlpThread.h"

using std::string;
using std::vector;

// our command line options
typedef struct {
  string services;
  ola::log_level log_level;
  unsigned short refresh;
  bool help;
} options;


// globals
ola::network::SelectServer ss;

// Called when we receive a new url list.
void DiscoveryDone(bool ok, const std::vector<std::string> &urls) {
  OLA_INFO << "in discovery callback, state is " << ok;

  vector<string>::const_iterator iter;
  for (iter = urls.begin(); iter != urls.end(); ++iter) {
    OLA_INFO << "  " << *iter;
  }
}


/*
 * Parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {"refresh", required_argument, 0, 'r'},
      {0, 0, 0, 0}
    };

  opts->log_level = ola::OLA_LOG_WARN;
  opts->refresh = 60;
  opts->help = false;

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "l:hr:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'h':
        opts->help = true;
        break;
      case 'l':
        switch (atoi(optarg)) {
          case 0:
            // nothing is written at this level
            // so this turns logging off
            opts->log_level = ola::OLA_LOG_NONE;
            break;
          case 1:
            opts->log_level = ola::OLA_LOG_FATAL;
            break;
          case 2:
            opts->log_level = ola::OLA_LOG_WARN;
            break;
          case 3:
            opts->log_level = ola::OLA_LOG_INFO;
            break;
          case 4:
            opts->log_level = ola::OLA_LOG_DEBUG;
            break;
          default :
            break;
        }
        break;
      case 'r':
        opts->refresh = atoi(optarg);
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
  std::cout << "Usage: " << arg << "\n"
  "\n"
  "Locate E1.33 services.\n"
  "\n"
  "  -h, --help               Display this help message and exit.\n"
  "  -l, --log-level <level>  Set the loggging level 0 .. 4.\n"
  "  -r, --refresh <seconds>  How often to check for new/expired services.\n"
  << std::endl;
  exit(EX_USAGE);
}


/*
 * Terminate on interrupt.
 */
static void InteruptSignal(int signo) {
  signo = 0;
  ss.Terminate();
}


/**
 * Main
 */
int main(int argc, char *argv[]) {
  options opts;
  ParseOptions(argc, argv, &opts);

  if (opts.help)
    DisplayHelpAndExit(argv[0]);

  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);

  // signal handler
  struct sigaction act, oact;

  act.sa_handler = InteruptSignal;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if (sigaction(SIGINT, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGINT";
    return false;
  }

  SlpThread thread(&ss, ola::NewCallback(&DiscoveryDone), opts.refresh);

  if (!thread.Init()) {
    OLA_WARN << "Init failed";
    return 1;
  }

  thread.Start();
  thread.Discover();

  ss.Run();
  thread.Join();
}
