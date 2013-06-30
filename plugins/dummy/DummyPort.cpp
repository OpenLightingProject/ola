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
 *
 * DummyPort.cpp
 * The Dummy Port for ola
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "ola/rdm/DimmerResponder.h"
#include "ola/rdm/DummyResponder.h"
#include "ola/rdm/MovingLightResponder.h"
#include "ola/rdm/UIDAllocator.h"
#include "ola/rdm/UIDSet.h"
#include "ola/stl/STLUtils.h"
#include "plugins/dummy/DummyDevice.h"
#include "plugins/dummy/DummyPort.h"

namespace ola {
namespace plugin {
namespace dummy {

using std::auto_ptr;
using ola::rdm::DimmerResponder;
using ola::rdm::DummyResponder;
using ola::rdm::MovingLightResponder;

/**
 * Create a new DummyPort
 * @param parent the parent device for this port
 * @param id the ID of this port
 * @param device_count the number of fake RDM devices to create
 * @param subdevice_count the number of subdevices each fake device should
 *   have.
 */
DummyPort::DummyPort(DummyDevice *parent,
                     const Options &options,
                     unsigned int id)
    : BasicOutputPort(parent, id, true, true) {
  UID first_uid(OPEN_LIGHTING_ESTA_CODE, DummyPort::kStartAddress);
  ola::rdm::UIDAllocator allocator(first_uid);

  for (unsigned int i = 0; i < options.number_of_dummy_responders; i++) {
    auto_ptr<UID> uid(allocator.AllocateNext());
    if (!uid.get()) {
      OLA_WARN << "Insufficient UIDs to create dummy RDM devices";
      break;
    }
    STLReplaceAndDelete(&m_responders, *uid, new DummyResponder(*uid, 0));
  }

  for (unsigned int i = 0; i < options.number_of_dimmers; i++) {
    auto_ptr<UID> uid(allocator.AllocateNext());
    if (!uid.get()) {
      OLA_WARN << "Insufficient UIDs to create dummy RDM devices";
      break;
    }
    STLReplaceAndDelete(
        &m_responders, *uid,
        new DimmerResponder(*uid, options.dimmer_sub_device_count));
  }

  for (unsigned int i = 0; i < options.number_of_moving_lights; i++) {
    auto_ptr<UID> uid(allocator.AllocateNext());
    if (!uid.get()) {
      OLA_WARN << "Insufficient UIDs to create dummy RDM devices";
      break;
    }
    STLReplaceAndDelete(&m_responders, *uid, new MovingLightResponder(*uid));
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
  for (unsigned int i = 0; i < 10 && i < data.size(); i++)
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
  UID dest = request->DestinationUID();
  if (dest.IsBroadcast()) {
    broadcast_request_tracker *tracker = new broadcast_request_tracker;
    tracker->expected_count = m_responders.size();
    tracker->current_count = 0;
    tracker->failed = false;
    tracker->callback = callback;
    for (ResponderMap::iterator i = m_responders.begin();
         i != m_responders.end(); i++) {
      i->second->SendRDMRequest(
        request->Duplicate(),
        NewSingleCallback(this, &DummyPort::HandleBroadcastAck, tracker));
    }
    delete request;
  } else {
      ResponderMap::iterator i = m_responders.find(dest);
      if (i != m_responders.end()) {
        i->second->SendRDMRequest(request, callback);
      } else {
          std::vector<std::string> packets;
          callback->Run(ola::rdm::RDM_UNKNOWN_UID, NULL, packets);
          delete request;
      }
  }
}


void DummyPort::RunDiscovery(RDMDiscoveryCallback *callback) {
  ola::rdm::UIDSet uid_set;
  for (ResponderMap::iterator i = m_responders.begin();
    i != m_responders.end(); i++) {
    uid_set.AddUID(i->first);
  }
  callback->Run(uid_set);
}


void DummyPort::HandleBroadcastAck(broadcast_request_tracker *tracker,
                                   ola::rdm::rdm_response_code code,
                                   const ola::rdm::RDMResponse *response,
                                   const std::vector<std::string> &packets) {
  tracker->current_count++;
  if (code != ola::rdm::RDM_WAS_BROADCAST)
    tracker->failed = true;
  if (tracker->current_count == tracker->expected_count) {
    // all ports have completed
    tracker->callback->Run(
        tracker->failed ?  ola::rdm::RDM_FAILED_TO_SEND :
          ola::rdm::RDM_WAS_BROADCAST,
        NULL,
        packets);
    delete tracker;
  }
  (void) response;
}


DummyPort::~DummyPort() {
  for (ResponderMap::iterator i = m_responders.begin();
      i != m_responders.end(); i++) {
    delete i->second;
  }
}
}  // namespace dummy
}  // namespace plugin
}  // namespace ola
