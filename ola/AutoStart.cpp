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
 * AutoStart.cpp
 * Connects to the ola server, starting it if it's not already running.
 * Copyright (C) 2010 Simon Newton
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

// On MinGW, SocketAddress.h pulls in WinSock2.h, which needs to be after
// WinSock2.h, hence this order
#include <ola/network/SocketAddress.h>
#include <ola/AutoStart.h>

#ifdef _WIN32
#define VC_EXTRALEAN
#include <ola/win/CleanWindows.h>
#include <tchar.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif  // _WIN32

#include <ola/network/IPV4Address.h>
#include <ola/Logging.h>

namespace ola {
namespace client {

using ola::network::TCPSocket;

/*
 * Open a connection to the server.
 */
TCPSocket *ConnectToServer(unsigned short port) {
  ola::network::IPV4SocketAddress server_address(
      ola::network::IPV4Address::Loopback(), port);
  TCPSocket *socket = TCPSocket::Connect(server_address);
  if (socket)
    return socket;

  OLA_INFO << "Attempting to start olad";

#ifdef _WIN32
  // On Windows, olad is not (yet) available as a service, so we just launch
  // it as a normal process
  STARTUPINFO startup_info;
  PROCESS_INFORMATION process_information;

  memset(&startup_info, 0, sizeof(startup_info));
  startup_info.cb = sizeof(startup_info);
  memset(&process_information, 0, sizeof(process_information));

  // On unicode systems, cmd_line may be modified by CreateProcess, so we need
  // to create a copy
  TCHAR* cmd_line = _tcsdup(TEXT("olad --syslog"));

  if (!CreateProcess(NULL,
                     cmd_line,
                     NULL,
                     NULL,
                     FALSE,
                     CREATE_NEW_CONSOLE,
                     NULL,
                     NULL,
                     &startup_info,
                     &process_information)) {
    OLA_WARN << "Could not launch olad " << GetLastError();
    _exit(1);
  }

  // Don't leak the handles
  CloseHandle(process_information.hProcess);
  CloseHandle(process_information.hThread);

  free(cmd_line);

  // wait a bit here for the server to come up. Sleep time is in milliseconds.
  Sleep(1000);
#else
  pid_t pid = fork();
  if (pid < 0) {
    OLA_WARN << "Could not fork: " << strerror(errno);
    return NULL;
  } else if (pid == 0) {
    // fork again so the parent can call waitpid immediately.
    pid_t pid = fork();
    if (pid < 0) {
      OLA_WARN << "Could not fork: " << strerror(errno);
      _exit(1);
    } else if (pid > 0) {
      _exit(0);
    }

    // Try to start the server, we pass --daemon (fork into background) and
    // --syslog (log to syslog).
    execlp("olad", "olad", "--daemon", "--syslog",
           reinterpret_cast<char*>(NULL));
    OLA_WARN << "Failed to exec: " << strerror(errno);
    _exit(1);
  }

  if (waitpid(pid, NULL, 0) != pid)
    OLA_WARN << "waitpid error: " << strerror(errno);

  // wait a bit here for the server to come up
  sleep(1);
#endif  // _WIN32

  return TCPSocket::Connect(server_address);
}
}  // namespace client
}  // namespace ola
