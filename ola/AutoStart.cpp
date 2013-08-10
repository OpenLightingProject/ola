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
 * AutoStart.cpp
 * Connects to the ola server, starting it if it's not already running.
 * Copyright (C) 2010 Simon Newton
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ola/AutoStart.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/SocketAddress.h>
#include <ola/Logging.h>

namespace ola {
namespace client {


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

    // Try to start the server, we pass -f (fork into background) and -s (log
    // to syslog).
    execlp("olad", "olad", "--daemon", "--syslog", NULL);
    OLA_WARN << "Failed to exec: " << strerror(errno);
    _exit(1);
  }

  if (waitpid(pid, NULL, 0) != pid)
    OLA_WARN << "waitpid error: " << strerror(errno);

  // wait a bit here for the server to come up
  sleep(1);
  return TCPSocket::Connect(server_address);
}
}  // namespace client
}  // namespace ola
