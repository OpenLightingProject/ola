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
 * E131Device.cpp
 * An E1.31 device
 * Copyright (C) 2007-2009 Simon Newton
 *
 * Ids 0-3 : Input ports (recv dmx)
 * Ids 4-7 : Output ports (send dmx)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ola/Logging.h>
#include <olad/Plugin.h>
#include <olad/PluginAdaptor.h>
#include <olad/Preferences.h>

#include "E131Device.h"
#include "E131Port.h"
#include "e131/E131Node.h"

namespace ola {
namespace e131 {

const std::string E131Device::IP_KEY = "ip";


/*
 * Create a new device
 */
E131Device::E131Device(Plugin *owner, const string &name,
                       const ola::e131::CID &cid,
                       Preferences *preferences,
                       const PluginAdaptor *plugin_adaptor):
  Device(owner, name),
  m_preferences(preferences),
  m_plugin_adaptor(plugin_adaptor),
  m_node(NULL),
  m_enabled(false),
  m_cid(cid) {

}


/*
 * Start this device
 */
bool E131Device::Start() {
  if (m_enabled)
    return false;

  m_node = new E131Node(m_preferences->GetValue(IP_KEY), m_cid);

  if (!m_node->Start()) {
    delete m_node;
    m_node = NULL;
    DeleteAllPorts();
    return false;
  }

  for (unsigned int i = 0; i < NUMBER_OF_E131_PORTS; i++) {
    E131InputPort *input_port = new E131InputPort(this, i, m_node);
    AddPort(input_port);
    E131OutputPort *output_port = new E131OutputPort(this, i, m_node);
    AddPort(output_port);
  }

  m_plugin_adaptor->AddSocket(m_node->GetSocket());
  m_enabled = true;
  return true;
}


/*
 * Stop this device
 */
bool E131Device::Stop() {
  if (!m_enabled)
    return false;

  m_plugin_adaptor->RemoveSocket(m_node->GetSocket());
  DeleteAllPorts();
  m_node->Stop();
  delete m_node;
  m_node = NULL;
  m_enabled = false;
  return true;
}

} // e131
} // ola
