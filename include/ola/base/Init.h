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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Init.h
 * A grab bag of functions useful for programs.
 * Copyright (C) 2012 Simon Newton
 */

/**
 * @defgroup init Initialization
 * @brief Helper functions that should be called on startup. They do things
 * like install SEGV stack trace handlers, initialize the random number
 * generator etc.
 * @snippet
 *   @code
 *   AppInit(argc, argv)
 *   @endcode
 *
 * @addtogroup init
 * @{
 * @file Init.h
 * @brief A grab bag of functions useful for programs.
 * @}
 */

#ifndef INCLUDE_OLA_BASE_INIT_H_
#define INCLUDE_OLA_BASE_INIT_H_

#include <ola/ExportMap.h>
#include <ola/Callback.h>

namespace ola {
/**
 * @addtogroup init
 * @{
 */

/**
 * @brief Used to initialize a server. Installs the SEGV handler, initializes
 * the random number generator and populates the export map.
 * @param argc argument count
 * @param argv pointer to argument strings
 * @param export_map an optional pointer to an ExportMap
 * @return true on success and false otherwise
 * @note If you are a client/application then you must call AppInit().
 */
bool ServerInit(int argc, char *argv[], ExportMap *export_map);

/**
 * @brief Used to initialize a application. Installs the SEGV handler and
 * initializes the random number generator.
 * @param argc argument count
 * @param argv pointer to the argument strings
 * @return true on success and false otherwise
 * @note If you are a server then you must call ServerInit().
 */
bool AppInit(int argc, char *argv[]);

/**
 * @brief Install a signal
 * @param signal the signal number to catch
 * @param fp is a function pointer to the handler
 * @return true on success and false otherwise
 */
bool InstallSignal(int signal, void(*fp)(int signo));

// Methods you probably don't need to use

/**
 * @brief Install signal handlers to deal with SIGBUS & SIGSEGV
 * @return true on success and false otherwise
 */
bool InstallSEGVHandler();

/**
 * @brief Populate the ExportMap with a couple of basic variables
 * @param argc argument count
 * @param argv pointer to the arugment strings
 * @param export_map ExportMap to populate
 */
void InitExportMap(int argc, char* argv[], ExportMap *export_map);

/**
 * @brief Run as a daemon
 * @note This uses the logging system, so you really should have initialized
 * logging before calling this. However, Daemonize() also closes all open file
 * descriptors so logging to stdout/stderr will go to /dev/null after this call.
 * Therefore if running as a daemon, you should only use syslog logging.
 * See logging.
 * @sa @ref logging
 * @return 
 */
int Daemonise();
/**@}*/
}  // namespace ola
#endif  // INCLUDE_OLA_BASE_INIT_H_
