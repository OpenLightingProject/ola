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
 * AutoStart.h
 * Connects to the ola server, starting it if it's not already running.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef OLA_AUTOSTART_H_
#define OLA_AUTOSTART_H_

#include <ola/Constants.h>
#include <ola/network/TCPSocket.h>

namespace ola {
namespace client {

/*
 * Open a connection to the server.
 */
ola::network::TCPSocket *ConnectToServer(unsigned short port);
}  // namespace client
}  // namespace ola
#endif  // OLA_AUTOSTART_H_
