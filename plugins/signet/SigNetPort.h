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
 * SigNetPort.h
 * The SigNetInputPort and SigNetOutputPort classes.
 * Copyright (C) 2026 Peter Newman
 */

#ifndef PLUGINS_SIGNET_SIGNETPORT_H_
#define PLUGINS_SIGNET_SIGNETPORT_H_

#include <string>
#include <vector>
#include "olad/Port.h"
#include "plugins/signet/SigNetDevice.h"
#include "plugins/signet/SigNetNode.h"

namespace ola {
namespace plugin {
namespace signet {

class SigNetPortHelper {
 public:
  bool PreSetUniverse(Universe *old_universe, Universe *new_universe);
  std::string Description(Universe *universe) const;
};


/**
 * @brief The Input Port class, for receiving DMX via Sig-Net.
 */
class SigNetInputPort: public BasicInputPort {
 public:
  /**
   * Create an SigNetInputPort.
   * @param parent the parent device
   * @param port_id the id for this port
   * @param plugin_adaptor a PluginAdaptor object, used by the base class.
   * @param node the SigNetNode object to use
   */
  SigNetInputPort(SigNetDevice *parent,
                  unsigned int port_id,
                  PluginAdaptor *plugin_adaptor,
                  SigNetNode *node);

  /**
   * Just return our DmxBuffer.
   */
  const DmxBuffer &ReadDMX() const { return m_buffer; }

  /**
   * Called during the patch process, just before the Universe of this port
   * changes.
   */
  bool PreSetUniverse(Universe *old_universe, Universe *new_universe) {
    return m_helper.PreSetUniverse(old_universe, new_universe);
  }
  void PostSetUniverse(Universe *old_universe, Universe *new_universe);
  std::string Description() const {
    return m_helper.Description(GetUniverse());
  }

  bool SupportsPriorities() const { return true; }
  uint8_t InheritedPriority() const { return m_priority; }

 private:
  SigNetNode *m_node;
  DmxBuffer m_buffer;
  SigNetPortHelper m_helper;
  uint8_t m_priority;

  /**
   * This is called when we receive new DMX values via Sig-Net.
   */
  void NewDMXData(const DmxBuffer &data);
};


/**
 * The Output Port class, for sending DMX via Sig-Net.
 */
class SigNetOutputPort: public BasicOutputPort {
 public:
  /**
   * @brief Create an SigNetOutputPort.
   * @param device the parent device
   * @param port_id the id for this port
   * @param node the SigNetNode object to use
   */
  SigNetOutputPort(SigNetDevice *device,
                   unsigned int port_id,
                   SigNetNode *node);
  ~SigNetOutputPort();

  /**
   * Called during the patch process, just before the Universe of this port
   * changes.
   */
  bool PreSetUniverse(Universe *old_universe, Universe *new_universe) {
    return m_helper.PreSetUniverse(old_universe, new_universe);
  }
  void PostSetUniverse(Universe *old_universe, Universe *new_universe);
  std::string Description() const {
    return m_helper.Description(GetUniverse());
  }

  /**
   * @brief Send this DMX buffer using Sig-Net. The second argument (priority) is not
   * used.
   * @param buffer the DMX data
   */
  bool WriteDMX(const DmxBuffer &buffer, uint8_t) {
    // TODO(Peter): Handle priority
    if (GetUniverse()) {
      return m_node->SendDMX(GetUniverse()->UniverseId(), buffer);
    }
    return true;
  }

 private:
  SigNetNode *m_node;
  SigNetPortHelper m_helper;
};
}  // namespace signet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SIGNET_SIGNETPORT_H_
