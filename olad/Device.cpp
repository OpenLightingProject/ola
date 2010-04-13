/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Device.cpp
 * Base implementation of the device class.
 * Copyright (C) 2005-2008 Simon Newton
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/service.h>
#include <map>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "olad/Device.h"
#include "olad/Plugin.h"
#include "olad/Port.h"
#include "olad/Universe.h"

namespace ola {

using google::protobuf::RpcController;
using google::protobuf::Closure;
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
  Stop();
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
  GenericFetchPortsVector(ports, m_input_ports);
}


void Device::OutputPorts(vector<OutputPort*> *ports) const {
  GenericFetchPortsVector(ports, m_output_ports);
}

/*
 * Returns the InputPort with the id port_id
 */
InputPort *Device::GetInputPort(unsigned int port_id) const {
  map<unsigned int, InputPort*>::const_iterator iter =
    m_input_ports.find(port_id);

  if (iter == m_input_ports.end())
    return NULL;
  return iter->second;
}


/*
 * Returns the OutputPort with the id port_id
 */
OutputPort *Device::GetOutputPort(unsigned int port_id) const {
  map<unsigned int, OutputPort *>::const_iterator iter =
    m_output_ports.find(port_id);

  if (iter == m_output_ports.end())
    return NULL;
  return iter->second;
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
void Device::Configure(RpcController *controller,
                       const string &request,
                       string *response,
                       Closure *done) {
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

  typename map<unsigned int, PortClass*>::iterator iter =
    port_map->find(port->PortId());
  if (iter == port_map->end()) {
    pair<unsigned int, PortClass*> p(port->PortId(), port);
    port_map->insert(p);
    return true;
  } else if (iter->second != port) {
    OLA_WARN << "Attempt to insert a port but this port id is already " <<
      "associated with a diferent port.";
    return true;
  }
  return true;
}


template<class PortClass>
void Device::GenericFetchPortsVector(
    vector<PortClass*> *ports,
    const map<unsigned int, PortClass*> &port_map) const {
  typename map<unsigned int, PortClass*>::const_iterator iter;
  for (iter = port_map.begin(); iter != port_map.end(); ++iter) {
    ports->push_back(iter->second);
  }
}


template <class PortClass>
void Device::GenericDeletePort(PortClass *port) {
  Universe *universe = port->GetUniverse();
  if (universe)
    universe->RemovePort(port);
  delete port;
}
}  // ola
