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
 * Olad.cpp
 * Main file for olad, parses the options, forks if required and runs the
 * daemon.
 * Copyright (C) 2005-2007 Simon Newton
 *
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sysexits.h>
#include <unistd.h>
#include <iostream>
#include <string>

#include "ola/Logging.h"
#include "olad/OlaDaemon.h"

using ola::OlaDaemon;
using std::string;
using std::cout;
using std::cerr;
using std::endl;

// the daemon
OlaDaemon *olad;

// options struct
typedef struct {
  ola::log_level level;
  ola::log_output output;
  bool daemon;
  bool help;
  int httpd;
  int http_quit;
  int http_port;
  int rpc_port;
  string http_data_dir;
  string config_dir;
} ola_options;


/*
 * Print a stack trace on seg fault
 */
static void sig_segv(int signo) {
  cout << "Recieved SIGSEGV or SIGBUS" << endl;
  #ifdef HAVE_EXECINFO_H
  enum {STACK_SIZE = 64};
  void *array[STACK_SIZE];
  size_t size = backtrace(array, STACK_SIZE);

  backtrace_symbols_fd(array, size, STDERR_FILENO);
  #endif
  exit(EX_SOFTWARE);
  (void) signo;
}

/*
 * Terminate cleanly on interrupt
 */
static void sig_interupt(int signo) {
  olad->Terminate();
  (void) signo;
}

/*
 * Reload plugins
 */
static void sig_hup(int signo) {
  olad->ReloadPlugins();
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
 * Set up the interrupt signal
 *
 * @return true on success, false on failure
 */
static bool InstallSignals() {
  struct sigaction act, oact;

  act.sa_handler = sig_interupt;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if (sigaction(SIGINT, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGINT";
    return false;
  }

  if (sigaction(SIGTERM, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGTERM";
    return false;
  }

  act.sa_handler = sig_segv;
  if (sigaction(SIGSEGV, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGSEGV";
    return false;
  }
  if (sigaction(SIGBUS, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGSEGV";
    return false;
  }

  act.sa_handler = sig_hup;

  if (sigaction(SIGHUP, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGHUP";
    return false;
  }

  act.sa_handler = sig_user1;

  if (sigaction(SIGUSR1, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGUSR1";
    return false;
  }
  return true;
}


/*
 * Display the help message
 */
static void DisplayHelp() {
  cout <<
  "Usage: olad [options]\n"
  "\n"
  "Start the ola daemon.\n"
  "\n"
  "  -c, --config-dir         Path to the config directory\n"
  "  -d, --http-data-dir      Path to the static content.\n"
  "  -f, --daemon             Fork into background.\n"
  "  -h, --help               Display this help message and exit.\n"
  "  -l, --log-level <level>  Set the logging level 0 .. 4 .\n"
  "  -p, --http-port          Port to run the http server on (default " <<
    ola::OlaServer::DEFAULT_HTTP_PORT << ")\n" <<
  "  -r, --rpc-port           Port to listen for RPCs on (default " <<
    ola::OlaDaemon::DEFAULT_RPC_PORT << ")\n" <<
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
static bool ParseOptions(int argc, char *argv[], ola_options *opts) {
  static struct option long_options[] = {
      {"config-dir", required_argument, 0, 'c'},
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

  bool options_valid = true;
  int c, ll;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "c:d:fhl:p:sr:", long_options, &option_index);
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
      case '?':
        break;
      default:
       break;
    }
  }
  return options_valid;
}


/*
 * Run as a daemon, logging has been initialized when this is called.
 * This means that logging can't use any persistant FDs.
 */
static int Daemonise() {
  pid_t pid;
  unsigned int i;
  int fd0, fd1, fd2;
  struct rlimit rl;
  struct sigaction sa;

  if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
    OLA_FATAL << "Could not determine file limit";
    exit(EX_OSFILE);
  }

  // fork
  if ((pid = fork()) < 0) {
    OLA_FATAL << "Could not fork\n";
    exit(EX_OSERR);
  } else if (pid != 0) {
    exit(EX_OK);
  }

  // start a new session
  setsid();

  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGHUP, &sa, NULL) < 0) {
    OLA_FATAL << "Could not install signal\n";
    exit(EX_OSERR);
  }

  if ((pid= fork()) < 0) {
    OLA_FATAL << "Could not fork\n";
    exit(EX_OSERR);
  } else if (pid != 0) {
    exit(EX_OK);
  }

  // change the current working directory
  if (chdir("/") < 0) {
    OLA_FATAL << "Can't change directory to /";
    exit(EX_OSERR);
  }

  // close all fds
  if (rl.rlim_max == RLIM_INFINITY)
    rl.rlim_max = 1024;
  for (i = 0; i < rl.rlim_max; i++)
    close(i);

  // send stdout, in and err to /dev/null
  fd0 = open("/dev/null", O_RDWR);
  fd1 = dup(0);
  fd2 = dup(0);

  if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
    OLA_FATAL << "Unexpected file descriptors: " << fd0 << ", " << fd1 << ", "
      << fd2;
    exit(EX_OSERR);
  }

  return 0;
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
  opts->httpd = 1;
  opts->http_quit = 1;
  opts->http_port = ola::OlaServer::DEFAULT_HTTP_PORT;
  opts->rpc_port = ola::OlaDaemon::DEFAULT_RPC_PORT;
  opts->http_data_dir = "";
  opts->config_dir = "";

  if (!ParseOptions(argc, argv, opts)) {
    DisplayHelp();
    exit(EX_USAGE);
  }

  if (opts->help) {
    DisplayHelp();
    exit(EX_OK);
  }

  // setup the logging
  ola::InitLogging(opts->level, opts->output);

  if (opts->daemon)
    Daemonise();
}


static void InitExportMap(ola::ExportMap *export_map, int argc, char*argv[]) {
  struct rlimit rl;
  ola::StringVariable *var = export_map->GetStringVar("binary");
  var->Set(argv[0]);

  var = export_map->GetStringVar("cmd-line");

  std::stringstream out;
  for (int i = 1; i < argc; i++) {
    out << argv[i] << " ";
  }
  var->Set(out.str());

  var = export_map->GetStringVar("fd-limit");
  if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
    var->Set("undertermined");
  } else {
    std::stringstream out;
    out << rl.rlim_cur;
    var->Set(out.str());
  }
}

/*
 * Main
 *
 */
int main(int argc, char *argv[]) {
  ola_options opts;
  ola::ExportMap export_map;
  Setup(argc, argv, &opts);

  if (!geteuid()) {
    OLA_FATAL << "Attempting to run as root, aborting.";
    return -1;
  }

  InitExportMap(&export_map, argc, argv);

  if (!InstallSignals())
    OLA_WARN << "Failed to install signal handlers";

  ola::ola_server_options ola_options;
  ola_options.http_enable = opts.httpd;
  ola_options.http_enable_quit = opts.http_quit;
  ola_options.http_port = opts.http_port;
  ola_options.http_data_dir = opts.http_data_dir;

  olad = new OlaDaemon(ola_options, &export_map, opts.rpc_port,
                       opts.config_dir);
  bool ret = olad->Init();

  if (ret) {
    olad->Run();
  }
  delete olad;
  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
