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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * E133Receiver.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/acn/ACNVectors.h>
#include <ola/e133/E133Receiver.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/SocketAddress.h>

#include <memory>
#include <string>

#include "plugins/e131/e131/E133Inflator.h"
#include "plugins/e131/e131/E133StatusInflator.h"
#include "plugins/e131/e131/RDMInflator.h"
#include "plugins/e131/e131/RootInflator.h"
#include "plugins/e131/e131/UDPTransport.h"

namespace ola {
namespace e133 {

using ola::NewCallback;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::rdm::RDMResponse;
using std::auto_ptr;
using std::string;


/**
 * Create a new E133Receiver.
 * @param socket the UDP socket to read from
 * @param status_callback the callback to run when status messages are
 * received.
 * @param rdm_callback the callback to run when RDM messages are received.
 */
E133Receiver::E133Receiver(ola::network::UDPSocket *socket,
                           StatusCallback *status_callback,
                           RDMCallback *rdm_callback)
    : m_udp_socket(socket),
      m_status_callback(status_callback),
      m_rdm_callback(rdm_callback),
      m_root_inflator(new plugin::e131::RootInflator()),
      m_e133_inflator(new plugin::e131::E133Inflator()),
      m_rdm_inflator(new plugin::e131::RDMInflator()),
      m_e133_status_inflator(new plugin::e131::E133StatusInflator()),
      m_incoming_udp_transport(
          new plugin::e131::IncomingUDPTransport(
            m_udp_socket,
            m_root_inflator.get())) {
  m_root_inflator->AddInflator(m_e133_inflator.get());
  m_e133_inflator->AddInflator(m_rdm_inflator.get());
  m_e133_inflator->AddInflator(m_e133_status_inflator.get());

  m_rdm_inflator->SetRDMHandler(
      NewCallback(this, &E133Receiver::HandlePacket));
  m_e133_status_inflator->SetStatusHandler(
      NewCallback(this, &E133Receiver::HandleStatusMessage));

  m_udp_socket->SetOnData(
        NewCallback(m_incoming_udp_transport.get(),
                    &ola::plugin::e131::IncomingUDPTransport::Receive));
}


/**
 * Handle a E1.33 Status Message.
 */
void E133Receiver::HandleStatusMessage(
    const ola::plugin::e131::TransportHeader *transport_header,
    const ola::plugin::e131::E133Header *e133_header,
    uint16_t status_code,
    const string &description) {
  if (m_status_callback) {
    m_status_callback->Run(
        E133StatusMessage(
          transport_header->Source().Host(), e133_header->Endpoint(),
          e133_header->Sequence(), status_code, description));
  }
}


/**
 * Handle an RDM packet
 */
void E133Receiver::HandlePacket(
    const ola::plugin::e131::TransportHeader *transport_header,
    const ola::plugin::e131::E133Header *e133_header,
    const std::string &raw_response) {
  if (!m_rdm_callback)
    return;

  OLA_INFO << "Got E1.33 data from " << transport_header->Source();

  // Attempt to unpack as a response for now.
  ola::rdm::rdm_response_code response_code;
  const RDMResponse *response = RDMResponse::InflateFromData(
    reinterpret_cast<const uint8_t*>(raw_response.data()),
    raw_response.size(),
    &response_code);

  if (!response) {
    OLA_WARN << "Failed to unpack E1.33 RDM message, ignoring request.";
    return;
  }

  m_rdm_callback->Run(E133RDMMessage(
      transport_header->Source().Host(), e133_header->Endpoint(),
      e133_header->Sequence(), response_code, response));
}
}  // namespace e133
}  // namespace ola
