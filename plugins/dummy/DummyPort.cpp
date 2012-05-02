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
 *
 * DummyPort.cpp
 * The Dummy Port for ola
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <iostream>
#include <string>
#include "ola/Logging.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/dummy/DummyPort.h"

namespace ola {
namespace plugin {
namespace dummy {


DummyPort::DummyPort(DummyDevice *parent, unsigned int id)
  : BasicOutputPort(parent, id, true) {
    for (unsigned int i = 0; i < DummyPort::NUMBER_OF_RESPONDERS; i++) {
      RESPONDER_UID uid(OPEN_LIGHTING_ESTA_CODE, DummyPort::START_ADDRESS + i);
      m_responders[uid] = new DummyResponder(uid);
    }
}


/*
 * Write operation
 * @param  data  pointer to the dmx data
 * @param  length  the length of the data
 */
bool DummyPort::WriteDMX(const DmxBuffer &buffer,
                         uint8_t priority) {
  (void) priority;
  m_buffer = buffer;
  stringstream str;
  std::string data = buffer.Get();

  str << "Dummy port: got " << buffer.Size() << " bytes: ";
  for (unsigned int i = 0;
       i < m_responders.begin()->second->Footprint() && i < data.size(); i++)
    str << "0x" << std::hex << 0 + (uint8_t) data.at(i) << " ";
  OLA_INFO << str.str();
  return true;
}


/*
 * This returns a single device
 */
void DummyPort::RunFullDiscovery(RDMDiscoveryCallback *callback) {
  RunDiscovery(callback);
}


/*
 * This returns a single device
 */
void DummyPort::RunIncrementalDiscovery(RDMDiscoveryCallback *callback) {
  RunDiscovery(callback);
}


/*
 * Handle an RDM Request
 */
void DummyPort::SendRDMRequest(const ola::rdm::RDMRequest *request,
                               ola::rdm::RDMCallback *callback) {
  RESPONDER_UID destUID = request->DestinationUID();
  if (m_responders.count(destUID) > 0) {
    m_responders[destUID]->SendRDMRequest(request, callback);
  }
}


void DummyPort::RunDiscovery(RDMDiscoveryCallback *callback) {
  ola::rdm::UIDSet uid_set;
  for (UID_RESPONDER_MAP::iterator i = m_responders.begin();
    i != m_responders.end(); i++) {
    uid_set.AddUID(i->second->UID());
  }
  callback->Run(uid_set);
}


DummyPort::~DummyPort() {
  if (!m_responders.empty()) {
    m_responders.clear();
  }
  delete &m_responders;
}
}  // dummy
}  // plugin
}  // ola
