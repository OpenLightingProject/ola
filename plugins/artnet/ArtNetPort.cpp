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
 * ArtNetPort.cpp
 * The ArtNet plugin for ola
 * Copyright (C) 2005 - 2009 Simon Newton
 */
#include <string.h>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "olad/Universe.h"
#include "plugins/artnet/ArtNetDevice.h"
#include "plugins/artnet/ArtNetPort.h"

namespace ola {
namespace plugin {
namespace artnet {

using ola::NewCallback;

/*
 * Reprogram our port.
 */
void ArtNetPortHelper::PostSetUniverse(Universe *new_universe,
                                       unsigned int port_id) {
  artnet_port_type direction = m_is_output ?
    ARTNET_INPUT_PORT : ARTNET_OUTPUT_PORT;

  uint8_t universe_id = new_universe ? new_universe->UniverseId() % 0x10 :
    ARTNET_DISABLE_PORT;

  m_node->SetPortUniverse(direction, port_id, universe_id);
}


/*
 * Return the port description
 */
string ArtNetPortHelper::Description(const Universe *universe,
                                     unsigned int port_id) const {
  if (!universe)
    return "";

  artnet_port_type direction = m_is_output ?
    ARTNET_INPUT_PORT : ARTNET_OUTPUT_PORT;

  std::stringstream str;
  str << "ArtNet Universe " <<
    static_cast<int>(m_node->NetAddress()) << ":" <<
    static_cast<int>(m_node->SubnetAddress()) << ":" <<
    static_cast<int>(m_node->GetPortUniverse(direction, port_id));
  return str.str();
}


/*
 * Set the DMX Handlers as needed
 */
void ArtNetInputPort::PostSetUniverse(Universe *old_universe,
                                      Universe *new_universe) {
  (void) old_universe;
  m_helper.PostSetUniverse(new_universe, PortId());

  if (new_universe && !old_universe) {
    m_helper.GetNode()->SetDMXHandler(
        PortId(),
        &m_buffer,
        NewCallback(
          static_cast<ola::BasicInputPort*>(this),
          &ArtNetInputPort::DmxChanged));
    m_helper.GetNode()->SetOutputPortRDMHandlers(
        PortId(),
        NewCallback(
          this,
          &ArtNetInputPort::RespondWithTod),
        NewCallback(
          this,
          &ArtNetInputPort::TriggerDiscovery),
        ola::NewCallback(
          static_cast<ola::BasicInputPort*>(this),
          &ArtNetInputPort::HandleRDMRequest));

  } else if (!new_universe) {
    m_helper.GetNode()->SetDMXHandler(PortId(), NULL, NULL);
    m_helper.GetNode()->SetOutputPortRDMHandlers(
        PortId(),
        NULL,
        NULL,
        NULL);
  }

  if (new_universe)
    TriggerRDMDiscovery(
        NewSingleCallback(this, &ArtNetInputPort::SendTODWithUIDs));
}


/*
 * Respond With Tod
 */
void ArtNetInputPort::RespondWithTod() {
  ola::rdm::UIDSet uids;
  if (GetUniverse())
    GetUniverse()->GetUIDs(&uids);
  SendTODWithUIDs(uids);
}


/**
 * Send a list of UIDs in a TOD
 */
void ArtNetInputPort::SendTODWithUIDs(const ola::rdm::UIDSet &uids) {
  m_helper.GetNode()->SendTod(PortId(), uids);
}


/**
 * Run the RDM discovery routine
 */
void ArtNetInputPort::TriggerDiscovery() {
  TriggerRDMDiscovery(
      NewSingleCallback(this, &ArtNetInputPort::SendTODWithUIDs));
}

/*
 * Write operation
 * @param data pointer to the dmx data
 * @param length the length of the data
 * @return true if the write succeeded, false otherwise
 */
bool ArtNetOutputPort::WriteDMX(const DmxBuffer &buffer,
                                uint8_t priority) {
  if (PortId() >= ARTNET_MAX_PORTS) {
    OLA_WARN << "Invalid artnet port id " << PortId();
    return false;
  }

  return m_helper.GetNode()->SendDMX(PortId(), buffer);
  (void) priority;
}


/*
 * Handle an RDMRequest
 */
void ArtNetOutputPort::SendRDMRequest(const ola::rdm::RDMRequest *request,
                                      ola::rdm::RDMCallback *on_complete) {
  // Discovery requests aren't proxied
  std::vector<std::string> packets;
  if (request->CommandClass() == RDMCommand::DISCOVER_COMMAND) {
    OLA_WARN << "Blocked attempt to send discovery command via Artnet";
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    delete request;
  } else {
    m_helper.GetNode()->SendRDMRequest(PortId(), request, on_complete);
  }
}


/*
 * Run the full RDM discovery process
 */
void ArtNetOutputPort::RunFullDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  m_helper.GetNode()->RunFullDiscovery(PortId(), callback);
}


/*
 * Run the incremental RDM discovery process
 */
void ArtNetOutputPort::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  // ArtNet nodes seem to run incremental discovery in the background. The
  // protocol doesn't provide a way of triggering incremental discovery so we
  // just do a full flush.
  m_helper.GetNode()->RunIncrementalDiscovery(PortId(), callback);
}


/*
 * Set the RDM handlers as appropriate
 */
void ArtNetOutputPort::PostSetUniverse(Universe *old_universe,
                                       Universe *new_universe) {
  m_helper.PostSetUniverse(new_universe, PortId());

  if (new_universe && !old_universe) {
    m_helper.GetNode()->SetUnsolicatedUIDSetHandler(
        PortId(),
        ola::NewCallback(
          static_cast<ola::BasicOutputPort*>(this),
          &ArtNetOutputPort::UpdateUIDs));
  } else if (!new_universe) {
    m_helper.GetNode()->SetUnsolicatedUIDSetHandler(PortId(), NULL);
  }
}
}  // artnet
}  // plugin
}  // ola
