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
 * StageProfiWidgetLan.cpp
 * StageProfi Lan Widget
 * Copyright (C) 2006 Simon Newton
 *
 * The StageProfi LAN Widget.
 */

#include <string>
#include "ola/Callback.h"
#include "ola/network/SocketAddress.h"
#include "ola/network/TCPSocket.h"
#include "plugins/stageprofi/StageProfiWidgetLan.h"

namespace ola {
namespace plugin {
namespace stageprofi {

using ola::network::IPV4Address;
using ola::network::TCPSocket;
using std::string;

/*
 * Connect to the widget
 * @returns true on success, false on failure
 */
bool StageProfiWidgetLan::Connect(const string &ip) {
  IPV4Address ip_address;
  if (!IPV4Address::FromString(ip, &ip_address))
    return false;

  m_socket = TCPSocket::Connect(
      ola::network::IPV4SocketAddress(ip_address, STAGEPROFI_PORT));

  if (m_socket)
    m_socket->SetOnData(
        NewCallback<StageProfiWidget>(this,
                                      &StageProfiWidget::SocketReady));
  m_device_path = ip;
  return m_socket;
}
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
