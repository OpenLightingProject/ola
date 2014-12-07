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
 * OPCPort.h
 * Ports for the Open Pixel Control plugin.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_OPENPIXELCONTROL_OPCPORT_H_
#define PLUGINS_OPENPIXELCONTROL_OPCPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/openpixelcontrol/OPCDevice.h"

namespace ola {
namespace plugin {
namespace openpixelcontrol {

/**
 * @brief An InputPort for the OPC plugin.
 *
 * OPCInputPorts correspond to a listening TCP socket.
 */
class OPCInputPort: public BasicInputPort {
 public:
  /**
   * @brief Create a new OPC Input Port.
   * @param parent the OPCDevice this port belongs to
   * @param channel the OPC channel for the port.
   * @param plugin_adaptor the PluginAdaptor to use
   * @param server the OPCServer to use, ownership is not transferred.
   */
  OPCInputPort(OPCServerDevice *parent,
               uint8_t channel,
               class PluginAdaptor *plugin_adaptor,
               class OPCServer *server);

  const DmxBuffer &ReadDMX() const { return m_buffer; }

  bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

  std::string Description() const;

 private:
  const uint8_t m_channel;
  class OPCServer* const m_server;
  DmxBuffer m_buffer;

  void NewData(uint8_t command, const uint8_t *data, unsigned int length);

  DISALLOW_COPY_AND_ASSIGN(OPCInputPort);
};

/**
 * @brief An OutputPort for the OPC plugin.
 */
class OPCOutputPort: public BasicOutputPort {
 public:
  /**
   * @brief Create a new OPC Output Port.
   * @param parent the OPCDevice this port belongs to
   * @param channel the OPC channel for the port.
   * @param client the OPCClient to use for this port, ownership is not
   *   transferred.
   */
  OPCOutputPort(OPCClientDevice *parent,
                uint8_t channel,
                class OPCClient *client);

  bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

  std::string Description() const;

 private:
  class OPCClient* const m_client;
  const uint8_t m_channel;

  DISALLOW_COPY_AND_ASSIGN(OPCOutputPort);
};
}  // namespace openpixelcontrol
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OPENPIXELCONTROL_OPCPORT_H_
