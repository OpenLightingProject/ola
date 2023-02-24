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
 * ShowNetPort.cpp
 * The ShowNet plugin for ola
 * Copyright (C) 2005 Simon Newton
 */
#include <sstream>
#include <string>

#include "ola/Callback.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "plugins/shownet/ShowNetDevice.h"
#include "plugins/shownet/ShowNetPort.h"

namespace ola {
namespace plugin {
namespace shownet {

using std::string;

string ShowNetInputPort::Description() const {
  std::ostringstream str;
  str << "ShowNet " << PortId() * DMX_UNIVERSE_SIZE + 1 << "-" <<
    (PortId() + 1) * DMX_UNIVERSE_SIZE;
  return str.str();
}


/*
 * Check for loops.
 */
bool ShowNetInputPort::PreSetUniverse(OLA_UNUSED Universe *old_universe,
                                      OLA_UNUSED Universe *new_universe) {
  AbstractDevice *device = GetDevice();
  OutputPort *output_port = device->GetOutputPort(PortId());
  if (output_port && output_port->GetUniverse()) {
    OLA_WARN << "Avoiding possible shownet loop on " << Description();
    return false;
  }
  return true;
}


/*
 * We intecept this to setup/remove the dmx handler
 */
void ShowNetInputPort::PostSetUniverse(Universe *old_universe,
                                       Universe *new_universe) {
  if (old_universe)
    m_node->RemoveHandler(PortId());

  if (new_universe)
    m_node->SetHandler(
        PortId(),
        &m_buffer,
        ola::NewCallback<ShowNetInputPort, void>(this,
                                                &ShowNetInputPort::DmxChanged));
}


string ShowNetOutputPort::Description() const {
  std::ostringstream str;
  str << "ShowNet " << PortId() * DMX_UNIVERSE_SIZE + 1 << "-" <<
    (PortId() + 1) * DMX_UNIVERSE_SIZE;
  return str.str();
}


/*
 * Check for loops.
 */
bool ShowNetOutputPort::PreSetUniverse(OLA_UNUSED Universe *old_universe,
                                       OLA_UNUSED Universe *new_universe) {
  AbstractDevice *device = GetDevice();
  InputPort *input_port = device->GetInputPort(PortId());
  if (input_port && input_port->GetUniverse()) {
    OLA_WARN << "Avoiding possible shownet loop on " << Description();
    return false;
  }
  return true;
}


bool ShowNetOutputPort::WriteDMX(const DmxBuffer &buffer,
                                 OLA_UNUSED uint8_t priority) {
  return !m_node->SendDMX(PortId(), buffer);
}
}  // namespace shownet
}  // namespace plugin
}  // namespace ola
