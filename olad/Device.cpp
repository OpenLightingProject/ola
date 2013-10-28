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
 * Device.cpp
 * Base implementation of the device class.
 * Copyright (C) 2005-2008 Simon Newton
 */
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "common/rpc/RpcController.h"
#include "common/rpc/RpcService.h"
#include "ola/Logging.h"
#include "ola/stl/STLUtils.h"
#include "olad/Device.h"
#include "olad/Plugin.h"
#include "olad/Port.h"
#include "olad/Universe.h"

namespace ola {

using std::pair;

/*
 * Create a new device
 * @param  owner  the plugin that owns this device
 * @param  name  a nice name for this device
 */
Device::Device(AbstractPlugin *owner, const string &name)
    : AbstractDevice(),
      m_enabled(false),
      m_owner(owner),
      m_name(name) {
}


/*
 * Stop this device
 */
Device::~Device() {
  // we can't call stop from here because it uses virtual methods
  if (m_enabled)
    OLA_FATAL << "Device " << m_name << " wasn't stopped before deleting, " <<
      "this represents a serious programming error.";
}


/*
 * Start the Device
 */
bool Device::Start() {
  if (m_enabled)
    return true;

  bool ret = StartHook();
  if (ret)
    m_enabled = true;
  return ret;
}


/*
 * Stop this device
 */
bool Device::Stop() {
  if (!m_enabled)
    return true;

  PrePortStop();
  DeleteAllPorts();
  PostPortStop();

  m_enabled = false;
  return true;
}


/*
 * Add a port to this device
 * @param port  the port to add
 * @return true on success, false if the port already exists
 */
bool Device::AddPort(InputPort *port) {
  return GenericAddPort(port, &m_input_ports);
}


/*
 * Add a port to this device
 * @param port  the port to add
 * @return 0 on success, non 0 on failure
 */
bool Device::AddPort(OutputPort *port) {
  return GenericAddPort(port, &m_output_ports);
}

void Device::InputPorts(vector<InputPort*> *ports) const {
  STLValues(m_input_ports, ports);
}


void Device::OutputPorts(vector<OutputPort*> *ports) const {
  STLValues(m_output_ports, ports);
}

/*
 * Returns the InputPort with the id port_id
 */
InputPort *Device::GetInputPort(unsigned int port_id) const {
  return STLFindOrNull(m_input_ports, port_id);
}


/*
 * Returns the OutputPort with the id port_id
 */
OutputPort *Device::GetOutputPort(unsigned int port_id) const {
  return STLFindOrNull(m_output_ports, port_id);
}


/*
 * Delete all ports and clear the port list
 */
void Device::DeleteAllPorts() {
  map<unsigned int, InputPort*>::iterator input_iter;
  map<unsigned int, OutputPort*>::iterator output_iter;
  for (input_iter = m_input_ports.begin(); input_iter != m_input_ports.end();
       ++input_iter) {
    GenericDeletePort(input_iter->second);
  }
  for (output_iter = m_output_ports.begin();
       output_iter != m_output_ports.end(); ++output_iter) {
    GenericDeletePort(output_iter->second);
  }
  m_input_ports.clear();
  m_output_ports.clear();
}


/*
 * Returns a unique id for this device
 */
string Device::UniqueId() const {
  if (m_unique_id.empty()) {
    if (!Owner()) {
      OLA_WARN << "Device: " << Name() << " missing owner";
      return "";
    }

    std::stringstream str;
    str << Owner()->Id() << "-" << DeviceId();
    m_unique_id = str.str();
  }
  return m_unique_id;
}

/*
 * Device Config request
 */
void Device::Configure(ola::rpc::RpcController *controller,
                       const string &request,
                       string *response,
                       Callback0<void> *done) {
  controller->SetFailed("Not Implemented");
  done->Run();
  (void) request;
  (void) response;
}


template<class PortClass>
bool Device::GenericAddPort(PortClass *port,
                            map<unsigned int, PortClass*> *port_map) {
  if (!port)
    return false;

  if (!STLInsertIfNotPresent(port_map, port->PortId(), port)) {
    OLA_WARN << "Attempt to insert a port but this port id is already " <<
      "associated with a diferent port.";
  }
  return true;
}


template <class PortClass>
void Device::GenericDeletePort(PortClass *port) {
  Universe *universe = port->GetUniverse();
  if (universe)
    universe->RemovePort(port);
  delete port;
}
}  // namespace ola
