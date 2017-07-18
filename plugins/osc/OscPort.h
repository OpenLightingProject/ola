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
 * OscPort.h
 * The OscInputPort and OscOutputPort classes.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef PLUGINS_OSC_OSCPORT_H_
#define PLUGINS_OSC_OSCPORT_H_

#include <string>
#include <vector>
#include "olad/Port.h"
#include "plugins/osc/OscAddressTemplate.h"
#include "plugins/osc/OscDevice.h"
#include "plugins/osc/OscNode.h"

namespace ola {
namespace plugin {
namespace osc {

/**
 * @brief The Input Port class, for receiving DMX via OSC.
 *
 * Note that the description of the port may change as it's patched and
 * unpatched from a universe (since the description can contain %d). Therefore
 * we store the description as a template, as well as the current value.
 */
class OscInputPort: public BasicInputPort {
 public:
  /**
   * Create an OscInputPort.
   * @param parent the parent device
   * @param port_id the id for this port
   * @param plugin_adaptor a PluginAdaptor object, used by the base class.
   * @param node the OscNode object to use
   * @param address the OSC address string for this port
   */
  OscInputPort(OscDevice *parent,
               unsigned int port_id,
               PluginAdaptor *plugin_adaptor,
               OscNode *node,
               const std::string &address);

  /**
   * Just return our DmxBuffer.
   */
  const DmxBuffer &ReadDMX() const { return m_buffer; }

  /**
   * Called during the patch process, just before the Universe of this port
   * changes.
   */
  bool PreSetUniverse(Universe *old_universe, Universe *new_universe);

  /**
   * Return the actual description
   */
  std::string Description() const { return m_actual_address; }

 private:
  OscNode *m_node;
  DmxBuffer m_buffer;
  const std::string m_address;
  std::string m_actual_address;

  /**
   * This is called when we receive new DMX values via OSC.
   */
  void NewDMXData(const DmxBuffer &data);
};


/**
 * The Output Port class, for sending DMX via OSC.
 */
class OscOutputPort: public BasicOutputPort {
 public:
  /**
   * @brief Create an OscOutputPort.
   * @param device the parent device
   * @param port_id the id for this port
   * @param node the OscNode object to use
   * @param targets the OSC targets to send to
   * @param data_format the format of OSC to send
   */
  OscOutputPort(OscDevice *device,
                unsigned int port_id,
                OscNode *node,
                const std::vector<OscTarget> &targets,
                OscNode::DataFormat data_format);
  ~OscOutputPort();

  /**
   * Called during the patch process, just before the Universe of this port
   * changes.
   */
  bool PreSetUniverse(Universe *old_universe, Universe *new_universe);

  /**
   * @brief Send this DMX buffer using OSC. The second argument (priority) is not
   * used.
   * @param buffer the DMX data
   */
  bool WriteDMX(const DmxBuffer &buffer, uint8_t) {
    return m_node->SendData(this->PortId(), m_data_format, buffer);
  }

  /**
   * Return the description for this port
   */
  std::string Description() const { return m_description; }

 private:
  OscNode *m_node;
  const std::vector<OscTarget> m_template_targets;
  std::vector<OscTarget> m_registered_targets;
  std::string m_description;
  OscNode::DataFormat m_data_format;

  void RemoveTargets();
  void SetUnpatchedDescription();
};
}  // namespace osc
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OSC_OSCPORT_H_
