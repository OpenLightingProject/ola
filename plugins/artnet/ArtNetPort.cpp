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
 * ArtNetPort.cpp
 * The Art-Net plugin for ola
 * Copyright (C) 2005 Simon Newton
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
using ola::rdm::RDMCommand;
using std::string;
using std::vector;

namespace {
static const uint8_t ARTNET_UNIVERSE_COUNT = 16;
};  // namespace

void ArtNetInputPort::PostSetUniverse(Universe *old_universe,
                                      Universe *new_universe) {
  if (new_universe) {
    m_node->SetOutputPortUniverse(
        PortId(), new_universe->UniverseId() % ARTNET_UNIVERSE_COUNT);
  } else {
    m_node->DisableOutputPort(PortId());
  }

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

  if (new_universe) {
    TriggerRDMDiscovery(
        NewSingleCallback(this, &ArtNetInputPort::SendTODWithUIDs));
  }
}

void ArtNetInputPort::RespondWithTod() {
  ola::rdm::UIDSet uids;
  if (GetUniverse()) {
    GetUniverse()->GetUIDs(&uids);
  }
  SendTODWithUIDs(uids);
}


string ArtNetInputPort::Description() const {
  if (!GetUniverse()) {
    return "";
  }

  std::ostringstream str;
  str << "Art-Net Universe "
      << static_cast<int>(m_node->NetAddress()) << ":"
      << static_cast<int>(m_node->SubnetAddress()) << ":"
      << static_cast<int>(m_node->GetOutputPortUniverse(PortId()));
  return str.str();
}

void ArtNetInputPort::SendTODWithUIDs(const ola::rdm::UIDSet &uids) {
  m_node->SendTod(PortId(), uids);
}

void ArtNetInputPort::TriggerDiscovery() {
  TriggerRDMDiscovery(
      NewSingleCallback(this, &ArtNetInputPort::SendTODWithUIDs));
}

bool ArtNetOutputPort::WriteDMX(const DmxBuffer &buffer,
                                OLA_UNUSED uint8_t priority) {
  if (PortId() >= ARTNET_MAX_PORTS) {
    OLA_WARN << "Invalid Art-Net port id " << PortId();
    return false;
  }

  return m_node->SendDMX(PortId(), buffer);
}

void ArtNetOutputPort::SendRDMRequest(ola::rdm::RDMRequest *request,
                                      ola::rdm::RDMCallback *on_complete) {
  // Discovery requests aren't proxied
  if (request->CommandClass() == RDMCommand::DISCOVER_COMMAND) {
    OLA_WARN << "Blocked attempt to send discovery command via Art-Net";
    ola::rdm::RunRDMCallback(on_complete,
                             ola::rdm::RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED);
    delete request;
  } else {
    m_node->SendRDMRequest(PortId(), request, on_complete);
  }
}

void ArtNetOutputPort::RunFullDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  m_node->RunFullDiscovery(PortId(), callback);
}

void ArtNetOutputPort::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  // Art-Net nodes seem to run incremental discovery in the background. The
  // protocol doesn't provide a way of triggering incremental discovery so we
  // just do a full flush.
  m_node->RunIncrementalDiscovery(PortId(), callback);
}

void ArtNetOutputPort::PostSetUniverse(Universe *old_universe,
                                       Universe *new_universe) {
  if (new_universe) {
    m_node->SetInputPortUniverse(
        PortId(), new_universe->UniverseId() % ARTNET_UNIVERSE_COUNT);
  } else {
    m_node->DisableInputPort(PortId());
  }

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
  if (!GetUniverse()) {
    return "";
  }

  std::ostringstream str;
  str << "Art-Net Universe "
      << static_cast<int>(m_node->NetAddress()) << ":"
      << static_cast<int>(m_node->SubnetAddress()) << ":"
      << static_cast<int>(m_node->GetInputPortUniverse(PortId()));
  return str.str();
}
}  // namespace artnet
}  // namespace plugin
}  // namespace ola
