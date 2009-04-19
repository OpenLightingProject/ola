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
 * Llad.cpp
 * Main file for llad, parses the options, forks if required and runs the
 * daemon.
 * Copyright (C) 2005-2007 Simon Newton
 *
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/resource.h>
#include <fcntl.h>

#include <iostream>
#include <lla/Logging.h>
#include "LlaDaemon.h"

using namespace std;
using lla::LlaDaemon;

// the daemon
LlaDaemon *llad;

// options struct
typedef struct {
  lla::log_level level;
  lla::log_output output;
  bool daemon;
  bool help;
  int httpd;
  int http_quit;
  int http_port;
  int rpc_port;
  string http_data_dir;
} lla_options;


/*
 * Terminate cleanly on interrupt
 */
static void sig_interupt(int signo) {
  signo = 0;
  llad->Terminate();
}

/*
 * Reload plugins
 */
static void sig_hup(int signo) {
  signo = 0;
  llad->ReloadPlugins();
}

/*
 * Change logging level
 *
 * need to fix race conditions here
 */
static void sig_user1(int signo) {
  signo = 0;
  lla::IncrementLogLevel();
}



/*
 * Set up the interrupt signal
 *
 * @return 0 on success, non 0 on failure
 */
static int InstallSignals() {
  struct sigaction act, oact;

  act.sa_handler = sig_interupt;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if (sigaction(SIGINT, &act, &oact) < 0) {
    LLA_WARN << "Failed to install signal SIGINT";
    return -1;
  }

  if (sigaction(SIGTERM, &act, &oact) < 0) {
    LLA_WARN << "Failed to install signal SIGTERM";
    return -1;
  }

  act.sa_handler = sig_hup;

  if (sigaction(SIGHUP, &act, &oact) < 0) {
    LLA_WARN << "Failed to install signal SIGHUP";
    return -1;
  }

  act.sa_handler = sig_user1;

  if (sigaction(SIGUSR1, &act, &oact) < 0) {
    LLA_WARN << "Failed to install signal SIGUSR1";
    return -1;
  }
  return 0;
}


/*
 * Display the help message
 */
static void DisplayHelp() {
  cout <<
  "Usage: llad [options] [--debug <level>]\n"
  "\n"
  "Start the lla daemon.\n"
  "\n"
  "  -d, --http-data-dir      Path to the static content.\n"
  "  -f, --daemon             Fork into background.\n"
  "  -h, --help               Display this help message and exit.\n"
  "  -l, --log-level <level>  Set the loggging level 0 .. 4 .\n"
  "  -p, --http-port          Port to run the http server on (default " <<
    lla::LlaServer::DEFAULT_HTTP_PORT << ")\n" <<
  "  -r, --rpc-port           Port to listen for RPCs on (default " <<
    lla::LlaDaemon::DEFAULT_RPC_PORT << ")\n" <<
  "  -s, --syslog             Log to syslog rather than stderr.\n"
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
static void ParseOptions(int argc, char *argv[], lla_options *opts) {
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"http-data-dir", required_argument, 0, 'd'},
      {"http-port", required_argument, 0, 'p'},
      {"log-level", required_argument, 0, 'l'},
      {"no-daemon", no_argument, 0, 'f'},
      {"no-http", no_argument, &opts->httpd, 0},
      {"no-http-quit", no_argument, &opts->http_quit, 0},
      {"rpc-port", required_argument, 0, 'r'},
      {"syslog", no_argument, 0, 's'},
      {0, 0, 0, 0}
    };

  int c, ll;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "l:p:fd:hsr:", long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
      case 0:
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

      case 's':
        opts->output = lla::LLA_LOG_SYSLOG;
        break;

      case 'l':
        ll = atoi(optarg);

        switch(ll) {
          case 0:
            // nothing is written at this level
            // so this turns logging off
            opts->level = lla::LLA_LOG_NONE;
            break;
          case 1:
            opts->level = lla::LLA_LOG_FATAL;
            break;
          case 2:
            opts->level = lla::LLA_LOG_WARN;
            break;
          case 3:
            opts->level = lla::LLA_LOG_INFO;
            break;
          case 4:
            opts->level = lla::LLA_LOG_DEBUG;
            break;
          default :
            break;
        }
        break;

      case 'p':
        opts->http_port = atoi(optarg);

      case 'r':
        opts->rpc_port = atoi(optarg);

      case '?':
        break;

      default:
       ;
    }
  }
}


/*
 * Run as a daemon
 */
static int Daemonise() {
  pid_t pid;
  unsigned int i;
  int fd0, fd1, fd2;
  struct rlimit rl;
  struct sigaction sa;

  if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
    cout << "Could not determine file limit" << endl;
    exit(1);
  }

  // fork
  if ((pid = fork()) < 0) {
    cout << "Could not fork\n" << endl;
    exit(1);
  } else if (pid != 0)
    exit(0);

  // start a new session
  setsid();

  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGHUP, &sa, NULL) < 0) {
    cout << "Could not install signal\n" << endl;
    exit(1);
  }

  if ((pid= fork()) < 0) {
    cout << "Could not fork\n" << endl;
    exit(1);
  } else if (pid != 0)
    exit(0);

  // close all fds
  if (rl.rlim_max == RLIM_INFINITY)
    rl.rlim_max = 1024;
  for (i=0; i < rl.rlim_max; i++)
    close(i);

  // send stdout, in and err to /dev/null
  fd0 = open("/dev/null", O_RDWR);
  fd1 = dup(0);
  fd2 = dup(0);

  return 0;
}


/*
 * Parse the options, and take action
 *
 * @param argc
 * @param argv
 * @param opts a pointer to the lla_options struct
 */
static void Setup(int argc, char*argv[], lla_options *opts) {
  opts->level = lla::LLA_LOG_WARN;
  opts->output = lla::LLA_LOG_STDERR;
  opts->daemon = false;
  opts->help = false;
  opts->httpd = 1;
  opts->http_quit = 1;
  opts->http_port = lla::LlaServer::DEFAULT_HTTP_PORT;
  opts->rpc_port = lla::LlaDaemon::DEFAULT_RPC_PORT;
  opts->http_data_dir = "";

  ParseOptions(argc, argv, opts);

  if(opts->help) {
    DisplayHelp();
    exit(0);
  }

  // setup the logging
  lla::InitLogging(opts->level, opts->output);

  if (opts->daemon)
    Daemonise();
}


static void InitExportMap(lla::ExportMap &export_map, int argc, char*argv[]) {
  struct rlimit rl;
  lla::StringVariable *var = export_map.GetStringVar("binary");
  var->Set(argv[0]);

  var = export_map.GetStringVar("cmd-line");

  stringstream out;
  for (int i = 1; i < argc; i++) {
    out << argv[i] << " ";
  }
  var->Set(out.str());

  var = export_map.GetStringVar("fd-limit");
  if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
    var->Set("undertermined");
  } else {
    stringstream out;
    out << rl.rlim_cur;
    var->Set(out.str());
  }
}

/*
 * Main
 *
 */
int main(int argc, char *argv[]) {
  lla_options opts;
  lla::ExportMap export_map;
  Setup(argc, argv, &opts);
  InitExportMap(export_map, argc, argv);

  if(InstallSignals())
    LLA_WARN << "Failed to install signal handlers";

  lla::lla_server_options lla_options;
  lla_options.http_enable = opts.httpd;
  lla_options.http_enable_quit = opts.http_quit;
  lla_options.http_port = opts.http_port;
  lla_options.http_data_dir = opts.http_data_dir;

  llad = new LlaDaemon(lla_options, &export_map, opts.rpc_port);

  if (llad->Init()) {
    llad->Run();
  }
  delete llad;
  return 0;
}
