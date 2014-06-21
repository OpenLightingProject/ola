/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Init.cpp
 * A grab bag of functions useful for programs.
 * Copyright (C) 2012 Simon Newton
 *
 * Maybe one day this will handle command line parsing as well. Wouldn't that
 * be nice.
 */

/**
 * @addtogroup init
 * @{
 * @file Init.cpp
 * @}
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#ifdef _WIN32
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Winsock2.h>
#else
#include <sys/resource.h>
#endif
#include <unistd.h>

#include <ola/ExportMap.h>
#include <ola/Logging.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/math/Random.h>

#include <iostream>
#include <string>

namespace ola {

/**
 * @addtogroup init
 * @{
 */

using std::cout;
using std::endl;


/**
 * @private
 * Print a stack trace on seg fault.
 */
static void _SIGSEGV_Handler(int signal) {
  cout << "Received SIGSEGV or SIGBUS" << endl;
  #ifdef HAVE_EXECINFO_H
  enum {STACK_SIZE = 64};
  void *array[STACK_SIZE];
  size_t size = backtrace(array, STACK_SIZE);

  backtrace_symbols_fd(array, size, STDERR_FILENO);
  #endif
  exit(EXIT_SOFTWARE);
  (void) signal;
}

bool ServerInit(int argc, char *argv[], ExportMap *export_map) {
  ola::math::InitRandom();
  if (!InstallSEGVHandler())
    return false;

  if (export_map)
    InitExportMap(argc, argv, export_map);
  return NetworkInit();
}


bool ServerInit(int *argc,
                char *argv[],
                ExportMap *export_map,
                const std::string &first_line,
                const std::string &description) {
  SetHelpString(first_line, description);
  ParseFlags(argc, argv);
  InitLoggingFromFlags();
  return ServerInit(*argc, argv, export_map);
}


bool AppInit(int *argc,
             char *argv[],
             const std::string &first_line,
             const std::string &description) {
  ola::math::InitRandom();
  SetHelpString(first_line, description);
  ParseFlags(argc, argv);
  InitLoggingFromFlags();
  if (!InstallSEGVHandler())
    return false;
  return NetworkInit();
}

#ifdef _WIN32
static void NetworkShutdown() {
  WSACleanup();
}
#endif

bool NetworkInit() {
#ifdef _WIN32
  WSADATA wsa_data;
  int result = WSAStartup(MAKEWORD(2, 0), &wsa_data);
  if (result != 0) {
    OLA_WARN << "WinSock initialization failed with " << result;
    return false;
  } else {
    atexit(NetworkShutdown);
    return true;
  }
#else
  return true;
#endif
}

bool InstallSignal(int signal, void(*fp)(int signo)) {
#ifdef _WIN32
  if (::signal(signal, fp) == SIG_ERR) {
    OLA_WARN << "Failed to install signal for " << signal;
    return false;
  }
#else
  struct sigaction action;
  action.sa_handler = fp;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;

  if (sigaction(signal, &action, NULL) < 0) {
    OLA_WARN << "Failed to install signal for " << signal;
    return false;
  }
#endif  // _WIN32
  return true;
}


bool InstallSEGVHandler() {
#ifndef _WIN32
  if (!InstallSignal(SIGBUS, _SIGSEGV_Handler)) {
    OLA_WARN << "Failed to install signal SIGBUS";
    return false;
  }
#endif  // !_WIN32
  if (!InstallSignal(SIGSEGV, _SIGSEGV_Handler)) {
    OLA_WARN << "Failed to install signal SIGSEGV";
    return false;
  }
  return true;
}


void InitExportMap(int argc, char* argv[], ExportMap *export_map) {
  ola::StringVariable *var = export_map->GetStringVar("binary");
  var->Set(argv[0]);

  var = export_map->GetStringVar("cmd-line");

  std::ostringstream out;
  for (int i = 1; i < argc; i++) {
    out << argv[i] << " ";
  }
  var->Set(out.str());

  var = export_map->GetStringVar("fd-limit");
#ifdef _WIN32
  {
    std::ostringstream out;
    out << _getmaxstdio();
    var->Set(out.str());
  }
#else
  struct rlimit rl;
  if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
    var->Set("undetermined");
  } else {
    std::ostringstream out;
    out << rl.rlim_cur;
    var->Set(out.str());
  }
#endif  // _WIN32
}


void Daemonise() {
#ifndef _WIN32
  pid_t pid;
  unsigned int i;
  int fd0, fd1, fd2;
  struct rlimit rl;
  struct sigaction sa;

  if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
    OLA_FATAL << "Could not determine file limit";
    exit(EXIT_OSFILE);
  }

  // fork
  if ((pid = fork()) < 0) {
    OLA_FATAL << "Could not fork\n";
    exit(EXIT_OSERR);
  } else if (pid != 0) {
    exit(EXIT_OK);
  }

  // start a new session
  setsid();

  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGHUP, &sa, NULL) < 0) {
    OLA_FATAL << "Could not install signal\n";
    exit(EXIT_OSERR);
  }

  if ((pid= fork()) < 0) {
    OLA_FATAL << "Could not fork\n";
    exit(EXIT_OSERR);
  } else if (pid != 0) {
    exit(EXIT_OK);
  }

  // change the current working directory
  if (chdir("/") < 0) {
    OLA_FATAL << "Can't change directory to /";
    exit(EXIT_OSERR);
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
    exit(EXIT_OSERR);
  }
#endif  // _WIN32
}
/**@}*/
}  // namespace ola
