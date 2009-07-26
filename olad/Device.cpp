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
#include <ola/Logging.h>
#include <olad/Device.h>
#include <olad/Port.h>
#include <olad/Universe.h>

namespace ola {

using google::protobuf::RpcController;
using google::protobuf::Closure;

/*
 * Create a new device
 *
 * @param  owner  the plugin that owns this device
 * @param  name  a nice name for this device
 */
Device::Device(AbstractPlugin *owner, const string &name):
  AbstractDevice(),
  m_enabled(false),
  m_owner(owner),
  m_name(name) {
}


/*
 * Stop this device
 */
Device::~Device() {
  if (m_enabled)
    Stop();
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
}


/*
 * Add a port to this device
 *
 * @param port  the port to add
 * @return 0 on success, non 0 on failure
 */
int Device::AddPort(AbstractPort *port) {
  m_ports.push_back(port);
  return 0;
}


/*
 * Returns a vector of ports in this device
 * @return a vector of pointers to AbstractPorts
 */
const vector<AbstractPort*> Device::Ports() const {
  return m_ports;
}


/*
 * Returns the Port with the id port_id
 */
AbstractPort *Device::GetPort(unsigned int port_id) const {
  if(port_id >= m_ports.size())
    return NULL;

  return m_ports[port_id];
}


/*
 * Delete all ports and clear the port list
 */
void Device::DeleteAllPorts() {
  vector<AbstractPort*>::iterator iter;
  for (iter = m_ports.begin(); iter != m_ports.end(); ++iter) {
    Universe *universe = (*iter)->GetUniverse();
    if (universe)
      universe->RemovePort(*iter);
    delete *iter;
  }
  m_ports.clear();
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

} //ola
