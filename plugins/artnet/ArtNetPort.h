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
 * ArtNetPort.h
 * The Art-Net plugin for ola
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_ARTNET_ARTNETPORT_H_
#define PLUGINS_ARTNET_ARTNETPORT_H_

#include <string>
#include "ola/rdm/RDMControllerInterface.h"
#include "olad/Port.h"
#include "plugins/artnet/ArtNetDevice.h"
#include "plugins/artnet/ArtNetNode.h"

namespace ola {
namespace plugin {
namespace artnet {

class ArtNetInputPort: public BasicInputPort {
 public:
  ArtNetInputPort(ArtNetDevice *parent,
                  unsigned int port_id,
                  class PluginAdaptor *plugin_adaptor,
                  ArtNetNode *node)
      : BasicInputPort(parent, port_id, plugin_adaptor, true),
        m_node(node) {}

  const DmxBuffer &ReadDMX() const { return m_buffer; }

  /**
   * Set the DMX Handlers as needed
   */
  void PostSetUniverse(Universe *old_universe, Universe *new_universe);

  /*
   * Respond With Tod
   */
  void RespondWithTod();

  std::string Description() const;

 private:
  DmxBuffer m_buffer;
  ArtNetNode *m_node;

  /**
   * Send a list of UIDs in a TOD
   */
  void SendTODWithUIDs(const ola::rdm::UIDSet &uids);

  /**
   * Run the RDM discovery routine
   */
  void TriggerDiscovery();
};

class ArtNetOutputPort: public BasicOutputPort {
 public:
  ArtNetOutputPort(ArtNetDevice *device,
                   unsigned int port_id,
                   ArtNetNode *node)
      : BasicOutputPort(device, port_id, true, true),
        m_node(node) {}

  bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

  /*
   * Handle an RDMRequest
   */
  void SendRDMRequest(ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *on_complete);

  /*
   * Run the full RDM discovery process
   */
  void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);

  /*
   * Run the incremental RDM discovery process
   */
  void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);

  /*
   * Set the RDM handlers as appropriate
   */
  void PostSetUniverse(Universe *old_universe, Universe *new_universe);

  std::string Description() const;

  // only the first output port supports timecode, otherwise we send it
  // multiple times.
  bool SupportsTimeCode() const {
    return PortId() == 0;
  }

  bool SendTimeCode(const ola::timecode::TimeCode &timecode) {
    return m_node->SendTimeCode(timecode);
  }

 private:
  ArtNetNode *m_node;
};
}  // namespace artnet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_ARTNET_ARTNETPORT_H_
