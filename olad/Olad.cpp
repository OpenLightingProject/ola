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
 * Olad.cpp
 * Main file for olad, parses the options, forks if required and runs the
 * daemon.
 * Copyright (C) 2005 Simon Newton
 *
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <memory>

// On MinGW, OlaDaemon.h pulls in SocketAddress.h which pulls in WinSock2.h,
// which needs to be after WinSock2.h, hence this order
#include "olad/OlaDaemon.h"

#include "ola/Logging.h"
#include "ola/base/Credentials.h"
#include "ola/base/Flags.h"
#include "ola/base/Init.h"
#include "ola/base/SysExits.h"
#include "ola/base/Version.h"
#include "ola/thread/SignalThread.h"

using ola::OlaDaemon;
using ola::thread::SignalThread;
using std::cout;
using std::endl;

DEFINE_default_bool(http, true, "Disable the HTTP server.");
DEFINE_default_bool(http_quit, true, "Disable the HTTP /quit handler.");
#ifndef _WIN32
DEFINE_s_default_bool(daemon, f, false,
                      "Fork and run as a background process.");
#endif  // _WIN32
DEFINE_s_string(http_data_dir, d, "", "The path to the static www content.");
DEFINE_s_string(interface, i, "",
                "The interface name (e.g. eth0) or IP address of the network "
                "interface to use for the web server.");
DEFINE_string(pid_location, "",
              "The directory containing the PID definitions.");
DEFINE_s_uint16(http_port, p, ola::OlaServer::DEFAULT_HTTP_PORT,
                "The port to run the HTTP server on. Defaults to 9090.");

/**
 * This is called by the SelectServer loop to start up the SignalThread. If the
 * thread fails to start, we terminate the SelectServer
 */
void StartSignalThread(ola::io::SelectServer *ss,
                       SignalThread *signal_thread) {
  if (!signal_thread->Start()) {
    ss->Terminate();
  }
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  // Take a copy of the arguments otherwise the export map is incorrect.
  const int original_argc = argc;
  char *original_argv[original_argc];
  for (int i = 0; i < original_argc; i++) {
    original_argv[i] = argv[i];
  }

  // We don't use the longer form for ServerInit here because we need to check
  // for root and possibly daemonise before doing the rest of the work from
  // ServerInit.

  ola::SetHelpString("[options]", "Start the OLA Daemon.");
  ola::ParseFlags(&argc, argv);

  ola::InitLoggingFromFlags();
  OLA_INFO << "OLA Daemon version " << ola::base::Version::GetVersion();

  #ifndef OLAD_SKIP_ROOT_CHECK
  uid_t uid;
  ola::GetEUID(&uid);
  if (ola::SupportsUIDs() && !uid) {
    OLA_FATAL << "Attempting to run as root, aborting.";
    return ola::EXIT_UNAVAILABLE;
  }
  #endif  // OLAD_SKIP_ROOT_CHECK

#ifndef _WIN32
  if (FLAGS_daemon)
    ola::Daemonise();
#endif  // _WIN32

  ola::ExportMap export_map;
  if (!ola::ServerInit(original_argc, original_argv, &export_map)) {
    return ola::EXIT_UNAVAILABLE;
  }

  // We need to block signals before we start any threads.
  // Signal setup is complex. First of all we need to install NULL handlers to
  // the signals are blocked before we start *any* threads. It's safest if we
  // do this before creating the OlaDaemon.
  SignalThread signal_thread;
  signal_thread.InstallSignalHandler(SIGINT, NULL);
  signal_thread.InstallSignalHandler(SIGTERM, NULL);
#ifndef _WIN32
  signal_thread.InstallSignalHandler(SIGHUP, NULL);
  signal_thread.InstallSignalHandler(
      SIGUSR1, ola::NewCallback(&ola::IncrementLogLevel));
#endif  // _WIN32

  ola::OlaServer::Options options;
  options.http_enable = FLAGS_http;
  options.http_enable_quit = FLAGS_http_quit;
  options.http_port = FLAGS_http_port;
  options.http_data_dir = FLAGS_http_data_dir.str();
  options.network_interface = FLAGS_interface.str();
  options.pid_data_dir = FLAGS_pid_location.str();

  std::auto_ptr<OlaDaemon> olad(new OlaDaemon(options, &export_map));
  if (!olad.get()) {
    return ola::EXIT_UNAVAILABLE;
  }

  // Now that the OlaDaemon has been created, we can reset the signal handlers
  // to do what we actually want them to.
  signal_thread.InstallSignalHandler(
      SIGINT,
      ola::NewCallback(olad->GetSelectServer(),
                       &ola::io::SelectServer::Terminate));
  signal_thread.InstallSignalHandler(
      SIGTERM,
      ola::NewCallback(olad->GetSelectServer(),
                       &ola::io::SelectServer::Terminate));

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

#ifndef _WIN32
  // Finally the OlaServer is not-null.
  signal_thread.InstallSignalHandler(
      SIGHUP,
      ola::NewCallback(olad->GetOlaServer(), &ola::OlaServer::ReloadPlugins));
#endif  // _WIN32

  olad->Run();
  return ola::EXIT_OK;
}
