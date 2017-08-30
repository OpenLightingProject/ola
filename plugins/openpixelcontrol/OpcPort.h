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
 * OpcPort.h
 * Ports for the Open Pixel Control plugin.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_OPENPIXELCONTROL_OPCPORT_H_
#define PLUGINS_OPENPIXELCONTROL_OPCPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/openpixelcontrol/OpcDevice.h"

namespace ola {
namespace plugin {
namespace openpixelcontrol {

/**
 * @brief An InputPort for the OPC plugin.
 *
 * OpcInputPorts correspond to a listening TCP socket.
 */
class OpcInputPort: public BasicInputPort {
 public:
  /**
   * @brief Create a new OPC Input Port.
   * @param parent the OpcDevice this port belongs to
   * @param channel the OPC channel for the port.
   * @param plugin_adaptor the PluginAdaptor to use
   * @param server the OpcServer to use, ownership is not transferred.
   */
  OpcInputPort(OpcServerDevice *parent,
               uint8_t channel,
               class PluginAdaptor *plugin_adaptor,
               class OpcServer *server);

  const DmxBuffer &ReadDMX() const { return m_buffer; }

  bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

  std::string Description() const;

 private:
  const uint8_t m_channel;
  class OpcServer* const m_server;
  DmxBuffer m_buffer;

  void NewData(uint8_t command, const uint8_t *data, unsigned int length);

  DISALLOW_COPY_AND_ASSIGN(OpcInputPort);
};

/**
 * @brief An OutputPort for the OPC plugin.
 */
class OpcOutputPort: public BasicOutputPort {
 public:
  /**
   * @brief Create a new OPC Output Port.
   * @param parent the OpcDevice this port belongs to
   * @param channel the OPC channel for the port.
   * @param client the OpcClient to use for this port, ownership is not
   *   transferred.
   */
  OpcOutputPort(OpcClientDevice *parent,
                uint8_t channel,
                class OpcClient *client);

  bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

  std::string Description() const;

 private:
  class OpcClient* const m_client;
  const uint8_t m_channel;

  DISALLOW_COPY_AND_ASSIGN(OpcOutputPort);
};
}  // namespace openpixelcontrol
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OPENPIXELCONTROL_OPCPORT_H_
