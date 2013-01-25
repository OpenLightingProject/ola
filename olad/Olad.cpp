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
 * Olad.cpp
 * Main file for olad, parses the options, forks if required and runs the
 * daemon.
 * Copyright (C) 2005-2007 Simon Newton
 *
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <string>

#include "ola/Logging.h"
#include "ola/base/Init.h"
#include "ola/base/Credentials.h"
#include "olad/OlaDaemon.h"

using ola::OlaDaemon;
using std::string;
using std::cout;
using std::cerr;
using std::endl;

// the daemon
OlaDaemon *global_olad = NULL;

// options struct
typedef struct {
  ola::log_level level;
  ola::log_output output;
  bool daemon;
  bool help;
  bool version;
  int httpd;
  int http_quit;
  int http_port;
  int rpc_port;
  string http_data_dir;
  string config_dir;
  string interface;
} ola_options;


/*
 * Terminate cleanly on interrupt
 */
static void sig_interupt(int signo) {
  if (global_olad)
    global_olad->Terminate();
  (void) signo;
}

/*
 * Reload plugins
 */
static void sig_hup(int signo) {
  if (global_olad)
    global_olad->ReloadPlugins();
  (void) signo;
}

/*
 * Change logging level
 *
 * need to fix race conditions here
 */
static void sig_user1(int signo) {
  ola::IncrementLogLevel();
  (void) signo;
}


/*
 * Set up the signal handlers.
 * @return true on success, false on failure
 */
static bool InstallSignals() {
  if (!ola::InstallSignal(SIGINT, sig_interupt))
    return false;

  if (!ola::InstallSignal(SIGTERM, sig_interupt))
    return false;

  if (!ola::InstallSignal(SIGHUP, sig_hup))
    return false;

  if (!ola::InstallSignal(SIGUSR1, sig_user1))
    return false;
  return true;
}


/*
 * Display the help message
 */
static void DisplayHelp() {
  cout <<
  "Usage: olad [options]\n"
  "\n"
  "Start the OLA Daemon.\n"
  "\n"
  "  -c, --config-dir         Path to the config directory\n"
  "  -d, --http-data-dir      Path to the static content.\n"
  "  -f, --daemon             Fork into background.\n"
  "  -h, --help               Display this help message and exit.\n"
  "  -i, --interface <interface name|ip> Network interface to use.\n"
  "  -l, --log-level <level>  Set the logging level 0 .. 4 .\n"
  "  -p, --http-port          Port to run the http server on (default " <<
    ola::OlaServer::DEFAULT_HTTP_PORT << ")\n" <<
  "  -r, --rpc-port           Port to listen for RPCs on (default " <<
    ola::OlaDaemon::DEFAULT_RPC_PORT << ")\n" <<
  "  -s, --syslog             Log to syslog rather than stderr.\n"
  "  -v, --version            Print the version number\n"
  "  --no-http                Don't run the http server\n"
  "  --no-http-quit           Disable the /quit handler\n"
  << endl;
}


/*
 * Parse the command line options
 *
 * @param argc
 * @param argv
 * @param opts  pointer to the options struct
 */
static bool ParseOptions(int argc, char *argv[], ola_options *opts) {
  static struct option long_options[] = {
      {"config-dir", required_argument, 0, 'c'},
      {"help", no_argument, 0, 'h'},
      {"http-data-dir", required_argument, 0, 'd'},
      {"http-port", required_argument, 0, 'p'},
      {"interface", required_argument, 0, 'i'},
      {"log-level", required_argument, 0, 'l'},
      {"daemon", no_argument, 0, 'f'},
      {"no-http", no_argument, &opts->httpd, 0},
      {"no-http-quit", no_argument, &opts->http_quit, 0},
      {"rpc-port", required_argument, 0, 'r'},
      {"syslog", no_argument, 0, 's'},
      {"version", no_argument, 0, 'v'},
      {0, 0, 0, 0}
    };

  bool options_valid = true;
  int c, ll;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc,
                    argv,
                    "c:d:fhi:l:p:r:sv",
                    long_options,
                    &option_index);
    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'c':
        opts->config_dir = optarg;
        break;
      case 'd':
        opts->http_data_dir = optarg;
        break;
      case 'f':
        opts->daemon = true;
        break;
      case 'h':
        opts->help = true;
        break;
      case 'i':
        opts->interface = optarg;
        break;
      case 's':
        opts->output = ola::OLA_LOG_SYSLOG;
        break;
      case 'l':
        ll = atoi(optarg);
        switch (ll) {
          case 0:
            // nothing is written at this level
            // so this turns logging off
            opts->level = ola::OLA_LOG_NONE;
            break;
          case 1:
            opts->level = ola::OLA_LOG_FATAL;
            break;
          case 2:
            opts->level = ola::OLA_LOG_WARN;
            break;
          case 3:
            opts->level = ola::OLA_LOG_INFO;
            break;
          case 4:
            opts->level = ola::OLA_LOG_DEBUG;
            break;
          default :
            cerr << "Invalid log level " << optarg << endl;
            options_valid = false;
            break;
        }
        break;
      case 'p':
        opts->http_port = atoi(optarg);
        break;
      case 'r':
        opts->rpc_port = atoi(optarg);
        break;
      case 'v':
        opts->version = true;
        break;
      case '?':
        break;
      default:
       break;
    }
  }
  return options_valid;
}


/*
 * Parse the options, and take action
 *
 * @param argc
 * @param argv
 * @param opts a pointer to the ola_options struct
 */
static void Setup(int argc, char*argv[], ola_options *opts) {
  opts->level = ola::OLA_LOG_WARN;
  opts->output = ola::OLA_LOG_STDERR;
  opts->daemon = false;
  opts->help = false;
  opts->version = false;
  opts->httpd = 1;
  opts->http_quit = 1;
  opts->http_port = ola::OlaServer::DEFAULT_HTTP_PORT;
  opts->rpc_port = ola::OlaDaemon::DEFAULT_RPC_PORT;
  opts->http_data_dir = "";
  opts->config_dir = "";
  opts->interface = "";

  if (!ParseOptions(argc, argv, opts)) {
    DisplayHelp();
    exit(EX_USAGE);
  }

  if (opts->help) {
    DisplayHelp();
    exit(EX_OK);
  }

  if (opts->version) {
    cout << "OLA Daemon version " << VERSION << endl;
    exit(EX_OK);
  }

  // setup the logging
  ola::InitLogging(opts->level, opts->output);
  OLA_INFO << "OLA Daemon version " << VERSION;

  if (opts->daemon)
    ola::Daemonise();
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola_options opts;
  ola::ExportMap export_map;
  Setup(argc, argv, &opts);

  #ifndef OLAD_SKIP_ROOT_CHECK
  if (!ola::GetEUID()) {
    OLA_FATAL << "Attempting to run as root, aborting.";
    return EX_UNAVAILABLE;
  }
  #endif

  ola::ServerInit(argc, argv, &export_map);

  if (!InstallSignals())
    OLA_WARN << "Failed to install signal handlers";

  ola::ola_server_options ola_options;
  ola_options.http_enable = opts.httpd;
  ola_options.http_enable_quit = opts.http_quit;
  ola_options.http_port = opts.http_port;
  ola_options.http_data_dir = opts.http_data_dir;
  ola_options.interface = opts.interface;

  std::auto_ptr<OlaDaemon> olad(
      new OlaDaemon(ola_options, &export_map, opts.rpc_port, opts.config_dir));
  if (olad.get() && olad->Init()) {
    global_olad = olad.get();
    olad->Run();
    return EX_OK;
  }
  global_olad = NULL;
  return EX_UNAVAILABLE;
}
