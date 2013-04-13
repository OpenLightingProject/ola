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
 * slp-thread.cpp
 * Copyright (C) 2011 Simon Newton
 *
 */
#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <getopt.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/base/Init.h>
#include <ola/io/SelectServer.h>
#include <ola/slp/URLEntry.h>
#include <signal.h>
#include <sysexits.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "tools/e133/OLASLPThread.h"
#ifdef HAVE_LIBSLP
#include "tools/e133/OpenSLPThread.h"
#endif
#include "tools/e133/SLPThread.h"

using ola::slp::URLEntries;
using std::auto_ptr;
using std::string;
using std::vector;

// our command line options
typedef struct {
  string services;
  bool use_openslp;
  ola::log_level log_level;
  uint16_t refresh;
  bool help;
} options;

// globals
ola::io::SelectServer ss;

// Called when we receive a new url list.
void DiscoveryDone(bool ok, const URLEntries &urls) {
  if (!ok) {
    OLA_WARN << "SLP discovery failed";
  } else if (urls.empty()) {
    OLA_INFO << "No services found";
  } else {
    URLEntries::const_iterator iter;
    for (iter = urls.begin(); iter != urls.end(); ++iter) {
      OLA_INFO << "  " << iter->url();
    }
  }
}

/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  enum {
    OPENSLP_OPTION = 256,
  };

  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {"refresh", required_argument, 0, 'r'},
#ifdef HAVE_LIBSLP
      {"openslp", no_argument, 0, OPENSLP_OPTION},
#endif
      {0, 0, 0, 0}
    };

  opts->log_level = ola::OLA_LOG_WARN;
  opts->refresh = 60;
  opts->help = false;
  opts->use_openslp = false;

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
      case OPENSLP_OPTION:
        opts->use_openslp = true;
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
  "  -l, --log-level <level>  Set the logging level 0 .. 4.\n"
  "  -r, --refresh <seconds>  How often to check for new/expired services.\n"
#ifdef HAVE_LIBSLP
  "  --openslp                 Use openslp rather than the OLA SLP server\n"
#endif
  << std::endl;
  exit(EX_USAGE);
}


/*
 * Terminate on interrupt.
 */
static void InteruptSignal(int signo) {
  ss.Terminate();
  (void) signo;
}


/**
 * Main
 */
int main(int argc, char *argv[]) {
  ola::AppInit(argc, argv);
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

  auto_ptr<BaseSLPThread> slp_thread;
  if (opts.use_openslp) {
#ifdef HAVE_LIBSLP
    slp_thread.reset(new OpenSLPThread(&ss, opts.refresh));
#else
    OLA_WARN << "openslp not installed";
    return false;
#endif
  } else {
    slp_thread.reset(new OLASLPThread(&ss, opts.refresh));
  }
  slp_thread->SetNewDeviceCallback(ola::NewCallback(&DiscoveryDone));

  if (!slp_thread->Init()) {
    OLA_WARN << "Init failed";
    return 1;
  }

  if (!slp_thread->Start()) {
    OLA_WARN << "SLPThread Start() failed";
    exit(EX_UNAVAILABLE);
  }

  ss.Run();
  slp_thread->Join(NULL);
}
