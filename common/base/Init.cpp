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
 * Init.cpp
 * A grab bag of functions useful for programs.
 * Copyright (C) 2012 Simon Newton
 *
 * Maybe one day this will handle command line parsing as well. Wouldn't that
 * be nice.
 */


#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/resource.h>
#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#endif
#include <unistd.h>

#include <ola/base/Init.h>
#include <ola/ExportMap.h>
#include <ola/Logging.h>

#include <iostream>


#ifndef HAVE_SYSEXITS_H
// Copied from the Berkeley sources.

#define EX_OK       0   /* successful termination */
#define EX__BASE    64  /* base value for error messages */
#define EX_USAGE    64  /* command line usage error */
#define EX_DATAERR  65  /* data format error */
#define EX_NOINPUT  66  /* cannot open input */
#define EX_NOUSER   67  /* addressee unknown */
#define EX_NOHOST   68  /* host name unknown */
#define EX_UNAVAILABLE  69  /* service unavailable */
#define EX_SOFTWARE 70  /* internal software error */
#define EX_OSERR    71  /* system error (e.g., can't fork) */
#define EX_OSFILE   72  /* critical OS file missing */
#define EX_CANTCREAT    73  /* can't create (user) output file */
#define EX_IOERR    74  /* input/output error */
#define EX_TEMPFAIL 75  /* temp failure; user is invited to retry */
#define EX_PROTOCOL 76  /* remote error in protocol */
#define EX_NOPERM   77  /* permission denied */
#define EX_CONFIG   78  /* configuration error */
#define EX__MAX 78  /* maximum listed value */
#endif


namespace ola {

using std::cout;
using std::endl;


/*
 * Print a stack trace on seg fault.
 */
static void _SIGSEGV_Handler(int) {
  cout << "Recieved SIGSEGV or SIGBUS" << endl;
  #ifdef HAVE_EXECINFO_H
  enum {STACK_SIZE = 64};
  void *array[STACK_SIZE];
  size_t size = backtrace(array, STACK_SIZE);

  backtrace_symbols_fd(array, size, STDERR_FILENO);
  #endif
  exit(EX_SOFTWARE);
}


/**
 * This does the following:
 *  Installs the SEGV handler.
 *  Populates the export map.
 *
 * @param argc the number of arguments on the cmd line
 * @param argv the command line arguments
 * @param export_map an optional pointer to an ExportMap
 */
bool ServerInit(int argc, char *argv[], ExportMap *export_map) {
  if (!InstallSEGVHandler())
    return false;

  if (export_map)
    InitExportMap(argc, argv, export_map);
  return true;
}


/**
 * This does the following:
 *  Intalls the SEGV handler.
 *
 * @param argc the number of arguments on the cmd line
 * @param argv the command line arguments
 */
bool AppInit(int, char *[]) {
  if (!InstallSEGVHandler())
    return false;
  return true;
}


/**
 * Install a signal handler.
 * @param signal the signal number to catch.
 * @param fp a function pointer to call.
 */
bool InstallSignal(int signal, void(*fp)(int)) {
  struct sigaction action;
  action.sa_handler = fp;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;

  if (sigaction(signal, &action, NULL) < 0) {
    OLA_WARN << "Failed to install signal for " << signal;
    return false;
  }
  return true;
}


/**
 * Install the SIGBUS & SIGSEGV handlers.
 */
bool InstallSEGVHandler() {
  if (!InstallSignal(SIGBUS, _SIGSEGV_Handler)) {
    OLA_WARN << "Failed to install signal SIGBUS";
    return false;
  }
  if (!InstallSignal(SIGSEGV, _SIGSEGV_Handler)) {
    OLA_WARN << "Failed to install signal SIGSEGV";
    return false;
  }
  return true;
}


/**
 * Populate the ExportMap with a couple of basic variables.
 */
void InitExportMap(int argc, char* argv[], ExportMap *export_map) {
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
 * Run as a daemon.
 * This uses the logging system, so you really should have initialized logging
 * before calling this. However, this also closes all open FDs so logging to
 * stdout / stderr will go to /dev/null after this call. Therefore if running
 * as a daemon, you should only use syslog logging.
 */
int Daemonise() {
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
}  // ola
