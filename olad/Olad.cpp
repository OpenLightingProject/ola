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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <memory>

#include "ola/Logging.h"
#include "ola/base/Credentials.h"
#include "ola/base/Flags.h"
#include "ola/base/Init.h"
#include "ola/base/SysExits.h"
#include "olad/OlaDaemon.h"

using ola::OlaDaemon;
using std::cout;
using std::endl;

// the daemon
OlaDaemon *global_olad = NULL;

DEFINE_bool(http, true, "Disable the HTTP server");
DEFINE_bool(http_quit, true, "Disable the HTTP /quit hanlder");
DEFINE_s_bool(daemon, f, false, "Fork and run in the background");
DEFINE_s_bool(version, v, false, "Print version information");
DEFINE_s_string(config_dir, c, "", "Path to the config directory");
DEFINE_s_string(http_data_dir, d, "", "Path to the static www content");
DEFINE_s_string(interface, i, "",
                "The interface name (e.g. eth0) or IP of the network interface "
                "to use");
DEFINE_s_uint16(http_port, p, ola::OlaServer::DEFAULT_HTTP_PORT,
                "Port to run the http server on");
DEFINE_s_uint16(rpc_port, r, ola::OlaDaemon::DEFAULT_RPC_PORT,
                "Port to listen for RPCs on");

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
 * Main
 */
int main(int argc, char *argv[]) {
  ola::SetHelpString("[options]", "Start the OLA Daemon.");
  ola::ParseFlags(&argc, argv);

  if (FLAGS_version) {
    cout << "OLA Daemon version " << VERSION << endl;
    exit(ola::EXIT_OK);
  }

  ola::InitLoggingFromFlags();
  OLA_INFO << "OLA Daemon version " << VERSION;

  #ifndef OLAD_SKIP_ROOT_CHECK
  if (!ola::GetEUID()) {
    OLA_FATAL << "Attempting to run as root, aborting.";
    return ola::EXIT_UNAVAILABLE;
  }
  #endif

  if (FLAGS_daemon)
    ola::Daemonise();

  ola::ExportMap export_map;
  ola::ServerInit(argc, argv, &export_map);

  if (!InstallSignals())
    OLA_WARN << "Failed to install signal handlers";

  ola::ola_server_options ola_options;
  ola_options.http_enable = FLAGS_http;
  ola_options.http_enable_quit = FLAGS_http_quit;
  ola_options.http_port = FLAGS_http_port;
  ola_options.http_data_dir = FLAGS_http_data_dir.str();
  ola_options.interface = FLAGS_interface.str();

  std::auto_ptr<OlaDaemon> olad(
      new OlaDaemon(ola_options, &export_map, FLAGS_rpc_port,
                    FLAGS_config_dir));
  if (olad.get() && olad->Init()) {
    global_olad = olad.get();
    olad->Run();
    return ola::EXIT_OK;
  }
  global_olad = NULL;
  return ola::EXIT_UNAVAILABLE;
}
