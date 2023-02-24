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
 * Init.h
 * A grab bag of functions useful for programs.
 * Copyright (C) 2012 Simon Newton
 */

/**
 * @defgroup init Initialization
 * @brief Functions called during program startup.
 *
 * Programs using the OLA libraries should call either ServerInit() or
 * AppInit(). There are also extra functions to help with installing signal
 * handlers and daemonizing a process.
 *
 * @examplepara
 *   @code
 *   // For client applications
 *   AppInit(argc, argv);
 *   // For server applications
 *   ServerInit(argc, argv, &export_map);
 *   @endcode
 *
 * @addtogroup init
 * @{
 * @file Init.h
 * @brief Functions called during program startup.
 * @}
 */

#ifndef INCLUDE_OLA_BASE_INIT_H_
#define INCLUDE_OLA_BASE_INIT_H_

#include <ola/ExportMap.h>
#include <ola/Callback.h>
#include <string>

namespace ola {
/**
 * @addtogroup init
 * @{
 */

/**
 * @brief Used to initialize a server.
 * @param argc argument count
 * @param argv pointer to argument strings
 * @param export_map an optional pointer to an ExportMap
 * @return true on success and false otherwise
 * @note If you are a client/application then call AppInit() instead.
 *
 * This does the following:
 *  - installs the SEGV handler
 *  - initializes the random number generator
 *  - sets the thread scheduling options
 *  - populates the export map
 *  - initializes the network stack (Windows only).
 */
bool ServerInit(int argc, char *argv[], ExportMap *export_map);

/**
 * @brief Used to initialize a server. Installs the SEGV handler, initializes
 * the random number generator and populates the export map. Also sets the help
 * string for the program, parses flags and initialises logging from flags.
 * @param argc argument count
 * @param argv pointer to argument strings
 * @param export_map an optional pointer to an ExportMap
 * @param first_line the initial line that is displayed in the help section.
 * This is displayed after argv[0].
 * @param description a multiline description of the program
 * @return true on success and false otherwise
 * @note If you are a client/application then call AppInit() instead.
 * @sa ola::SetHelpString ola::ParseFlags ola::InitLoggingFromFlags
 */
bool ServerInit(int *argc,
                char *argv[],
                ExportMap *export_map,
                const std::string &first_line,
                const std::string &description);

/**
 * @brief Used to initialize a application. Installs the SEGV handler and
 * initializes the random number generator, sets the help string for the
 * program, parses flags and initialises logging from flags.
 * @param argc argument count
 * @param argv pointer to the argument strings
 * @param first_line the initial line that is displayed in the help section.
 * This is displayed after argv[0].
 * @param description a multiline description of the program
 * @return true on success and false otherwise
 * @note If you are a server then call ServerInit() instead.
 * @sa ola::SetHelpString ola::ParseFlags ola::InitLoggingFromFlags
 */
bool AppInit(int *argc,
             char *argv[],
             const std::string &first_line,
             const std::string &description);

/**
 * @brief Perform platform-specific initialization of the networking subsystem.
 *
 * This method is called by ServerInit() and AppInit(), so you only need to
 * call this yourself if you're not using those.
 * @return true on success and false otherwise
 */
bool NetworkInit();

/**
 * @brief Install a signal handler.
 * @param signal the signal number to catch
 * @param fp is a function pointer to the handler
 * @return true on success and false otherwise
 */
bool InstallSignal(int signal, void(*fp)(int signo));

/**
 * @brief Install signal handlers to deal with SIGBUS & SIGSEGV.
 * @return true on success and false otherwise
 *
 * On receiving a SIGBUS or SIGSEGV a stack trace will be printed.
 */
bool InstallSEGVHandler();

/**
 * @brief Populate the ExportMap with a couple of basic variables.
 * @param argc argument count
 * @param argv pointer to the argument strings
 * @param export_map ExportMap to populate
 *
 * This is called by ServerInit(). It sets the following variables:
 *  - binary: name of the binary.
 *  - cmd-line: command line used to start the binary
 *  - fd-limit: the max number of file descriptors
 */
void InitExportMap(int argc, char* argv[], ExportMap *export_map);

/**
 * @brief Run as a daemon
 *
 * Daemonize logs messages if it fails, so it's best to initialize the
 * logging system (ola::InitLogging) before calling. However Daemonize() closes
 * all open file descriptors so stdout/stderr will point to /dev/null in the
 * daemon process. Therefore daemons should always use syslog logging.
 *
 * If we can't daemonize the process is terminated.
 *
 * See logging.
 * @sa @ref logging
 */
void Daemonise();

/**
 * @brief Logs status of clock capabilities
 */
void ClockInit();
/**@}*/
}  // namespace ola
#endif  // INCLUDE_OLA_BASE_INIT_H_
