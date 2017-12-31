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
 * NanoleafNode.cpp
 * A Nanoleaf node
 * Copyright (C) 2017 Peter Newman
 */

#include <math.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/network/SocketAddress.h"
#include "plugins/nanoleaf/NanoleafNode.h"

namespace ola {
namespace plugin {
namespace nanoleaf {

using ola::network::IPV4SocketAddress;
using ola::network::UDPSocket;
using std::auto_ptr;
using std::vector;

/*
 * Create a new Nanoleaf node.
 * @param ss a SelectServerInterface to use
 * @param socket a UDPSocket or Null. Ownership is transferred.
 */
NanoleafNode::NanoleafNode(ola::io::SelectServerInterface *ss,
                           std::vector<uint8_t> panels,
                           ola::network::UDPSocketInterface *socket)
    : m_running(false),
      m_ss(ss),
      m_panels(panels),
      m_output_stream(&m_output_queue),
      m_socket(socket) {
}



/*
 * Cleanup.
 */
NanoleafNode::~NanoleafNode() {
  Stop();
}


/*
 * Start this node.
 */
bool NanoleafNode::Start() {
  if (m_running) {
    return false;
  }

  if (!InitNetwork()) {
    return false;
  }
  m_running = true;
  return true;
}


/*
 * Stop this node.
 */
bool NanoleafNode::Stop() {
  if (!m_running) {
    return false;
  }

  m_ss->RemoveReadDescriptor(m_socket.get());
  m_socket.reset();
  m_running = false;
  return true;
}


/*
 * Send some DMX data
 */
bool NanoleafNode::SendDMX(const IPV4SocketAddress &target, const DmxBuffer &buffer) {
  if (!buffer.Size()) {
    OLA_DEBUG << "Not sending 0 length packet";
    return true;
  } else if (buffer.Size() < NANOLEAF_SLOTS_PER_PANEL) {
    OLA_INFO << "Insufficient DMX data, required " << NANOLEAF_SLOTS_PER_PANEL
             << ", got " << buffer.Size();
  }

  uint8_t panel_count = std::min(
      static_cast<uint8_t>(m_panels.size()),
      static_cast<uint8_t>(floor(buffer.Size() / NANOLEAF_SLOTS_PER_PANEL)));

  m_output_queue.Clear();
  m_output_stream << panel_count;
  for (uint8_t i = 0; i < panel_count; i++) {
    m_output_stream << m_panels[i] << NANOLEAF_FRAME_COUNT;
    m_output_stream.Write(buffer.GetRaw() + (i * NANOLEAF_SLOTS_PER_PANEL), NANOLEAF_SLOTS_PER_PANEL);
    m_output_stream << NANOLEAF_WHITE_LEVEL << NANOLEAF_TRANSITION_TIME;
  }

  // m_output_queue.Dump(&std::cerr);

  bool ok = m_socket->SendTo(&m_output_queue, target);
  if (!ok) {
    OLA_WARN << "Failed to send Nanoleaf packet";
  }

  if (!m_output_queue.Empty()) {
    OLA_WARN << "Failed to send complete Nanoleaf packet";
    m_output_queue.Clear();
  }
  return ok;
}


/*
 * Called when there is data on this socket. Right now we discard all packets.
 */
void NanoleafNode::SocketReady() {
  uint8_t packet[1500];
  ssize_t packet_size = sizeof(packet);
  ola::network::IPV4SocketAddress source;

  if (!m_socket->RecvFrom(reinterpret_cast<uint8_t*>(&packet),
                          &packet_size,
                          &source)) {
    return;
  }

  OLA_INFO << "Received Nanoleaf packet from " << source << ", discarding";
}


/*
 * Setup the networking components.
 */
bool NanoleafNode::InitNetwork() {
  std::auto_ptr<ola::network::UDPSocketInterface> socket(m_socket.release());

  if (!socket.get()) {
    socket.reset(new UDPSocket());
  }

  if (!socket->Init()) {
    OLA_WARN << "Socket init failed";
    return false;
  }

  //if (!socket->Bind(IPV4SocketAddress(IPV4Address::WildCard(), NANOLEAF_PORT))) {
  //  return false;
  //}

  // Do we need to call this if we don't bind?
  socket->SetOnData(NewCallback(this, &NanoleafNode::SocketReady));
  m_ss->AddReadDescriptor(socket.get());
  m_socket.reset(socket.release());
  return true;
}
}  // namespace nanoleaf
}  // namespace plugin
}  // namespace ola
