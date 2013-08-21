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
 * Set the DMX Handlers as needed
 */
void ArtNetInputPort::PostSetUniverse(Universe *old_universe,
                                      Universe *new_universe) {
  if (new_universe)
    m_node->SetOutputPortUniverse(PortId(), new_universe->UniverseId() % 0xf);
  else
    m_node->DisableOutputPort(PortId());

  if (new_universe && !old_universe) {
    m_node->SetDMXHandler(
        PortId(),
        &m_buffer,
        NewCallback(static_cast<ola::BasicInputPort*>(this),
                    &ArtNetInputPort::DmxChanged));
    m_node->SetOutputPortRDMHandlers(
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
    m_node->SetDMXHandler(PortId(), NULL, NULL);
    m_node->SetOutputPortRDMHandlers(PortId(), NULL, NULL, NULL);
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


string ArtNetInputPort::Description() const {
  if (!GetUniverse())
    return "";

  std::stringstream str;
  str << "ArtNet Universe " <<
    static_cast<int>(m_node->NetAddress()) << ":" <<
    static_cast<int>(m_node->SubnetAddress()) << ":" <<
    static_cast<int>(m_node->GetOutputPortUniverse(PortId()));
  return str.str();
}


/**
 * Send a list of UIDs in a TOD
 */
void ArtNetInputPort::SendTODWithUIDs(const ola::rdm::UIDSet &uids) {
  m_node->SendTod(PortId(), uids);
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

  return m_node->SendDMX(PortId(), buffer);
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
    m_node->SendRDMRequest(PortId(), request, on_complete);
  }
}


/*
 * Run the full RDM discovery process
 */
void ArtNetOutputPort::RunFullDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  m_node->RunFullDiscovery(PortId(), callback);
}


/*
 * Run the incremental RDM discovery process
 */
void ArtNetOutputPort::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  // ArtNet nodes seem to run incremental discovery in the background. The
  // protocol doesn't provide a way of triggering incremental discovery so we
  // just do a full flush.
  m_node->RunIncrementalDiscovery(PortId(), callback);
}


/*
 * Set the RDM handlers as appropriate
 */
void ArtNetOutputPort::PostSetUniverse(Universe *old_universe,
                                       Universe *new_universe) {
  if (new_universe)
    m_node->SetInputPortUniverse(
        PortId(), new_universe->UniverseId() % 0xf);
  else
    m_node->DisableInputPort(PortId());

  if (new_universe && !old_universe) {
    m_node->SetUnsolicitedUIDSetHandler(
        PortId(),
        ola::NewCallback(
            static_cast<ola::BasicOutputPort*>(this),
            &ArtNetOutputPort::UpdateUIDs));
  } else if (!new_universe) {
    m_node->SetUnsolicitedUIDSetHandler(PortId(), NULL);
  }
}

string ArtNetOutputPort::Description() const {
  if (!GetUniverse())
    return "";

  std::stringstream str;
  str << "ArtNet Universe " <<
    static_cast<int>(m_node->NetAddress()) << ":" <<
    static_cast<int>(m_node->SubnetAddress()) << ":" <<
    static_cast<int>(m_node->GetInputPortUniverse(PortId()));
  return str.str();
}
}  // namespace artnet
}  // namespace plugin
}  // namespace ola
