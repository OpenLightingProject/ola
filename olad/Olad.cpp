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
#include "ola/thread/SignalThread.h"
#include "olad/OlaDaemon.h"

using ola::OlaDaemon;
using ola::thread::SignalThread;
using std::cout;
using std::endl;

DEFINE_bool(http, true, "Disable the HTTP server");
DEFINE_bool(http_quit, true, "Disable the HTTP /quit hanlder");
DEFINE_s_bool(daemon, f, false, "Fork and run in the background");
DEFINE_s_bool(version, v, false, "Print version information");
DEFINE_s_string(http_data_dir, d, "", "Path to the static www content");
DEFINE_s_string(interface, i, "",
                "The interface name (e.g. eth0) or IP of the network interface "
                "to use");
DEFINE_string(pid_location, PID_DATA_DIR,
              "The directory containing the PID definitions");
DEFINE_s_uint16(http_port, p, ola::OlaServer::DEFAULT_HTTP_PORT,
                "Port to run the http server on");


/**
 * This is called by the SelectServer loop to start up the SignalThread. If the
 * thread fails to start, we terminate the SelectServer
 */
void StartSignalThread(ola::SelectServer *ss, SignalThread *signal_thread) {
  if (!signal_thread->Start()) {
    ss->Terminate();
  }
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

  // We need to block signals before we start any threads.
  // Signal setup is complex. First of all we need to install NULL handlers to
  // the signals are blocked before we start *any* threads. It's safest if we
  // do this before creating the OlaDaemon.
  SignalThread signal_thread;
  signal_thread.InstallSignalHandler(SIGINT, NULL);
  signal_thread.InstallSignalHandler(SIGTERM, NULL);
  signal_thread.InstallSignalHandler(SIGHUP, NULL);
  signal_thread.InstallSignalHandler(
      SIGUSR1, ola::NewCallback(&ola::IncrementLogLevel));

  ola::OlaServer::Options options;
  options.http_enable = FLAGS_http;
  options.http_enable_quit = FLAGS_http_quit;
  options.http_port = FLAGS_http_port;
  options.http_data_dir = FLAGS_http_data_dir.str();
  options.interface = FLAGS_interface.str();
  options.pid_data_dir = FLAGS_pid_location.str();

  std::auto_ptr<OlaDaemon> olad(new OlaDaemon(options, &export_map));
  if (!olad.get()) {
    return ola::EXIT_UNAVAILABLE;
  }

  // Now that the OlaDaemon has been created, we can reset the signal handlers
  // to do what we actually want them to.
  signal_thread.InstallSignalHandler(
      SIGINT,
      ola::NewCallback(olad->GetSelectServer(), &ola::SelectServer::Terminate));
  signal_thread.InstallSignalHandler(
      SIGTERM,
      ola::NewCallback(olad->GetSelectServer(), &ola::SelectServer::Terminate));

  // We can't start the signal thread here, otherwise there is a race
  // condition if a signal arrives before we enter the SelectServer Run()
  // method. Instead we schedule it to start from the SelectServer loop.
  olad->GetSelectServer()->Execute(ola::NewSingleCallback(
        &StartSignalThread,
        olad->GetSelectServer(),
        &signal_thread));

  if (!olad->Init()) {
    return ola::EXIT_UNAVAILABLE;
  }

  // Finally the OlaServer is not-null.
  signal_thread.InstallSignalHandler(
      SIGHUP,
      ola::NewCallback(olad->GetOlaServer(), &ola::OlaServer::ReloadPlugins));

  olad->Run();
  return ola::EXIT_OK;
}
