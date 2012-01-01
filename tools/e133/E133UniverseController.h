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
 * E133UniverseController.h
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/Clock.h>
#include <ola/network/IPV4Address.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/UID.h>

#include <deque>
#include <map>
#include <string>

#include "plugins/e131/e131/E133Header.h"
#include "plugins/e131/e131/E133Layer.h"
#include "plugins/e131/e131/TransportHeader.h"

#include "E133Component.h"

#ifndef TOOLS_E133_E133UNIVERSECONTROLLER_H_
#define TOOLS_E133_E133UNIVERSECONTROLLER_H_


using ola::network::IPV4Address;
using ola::rdm::UID;
using ola::plugin::e131::E133Layer;


/**
 * This holds everything we need to track about a pending E1.33 RDM request
 */
class PendingE133Request {
  public:
    PendingE133Request(const ola::rdm::RDMRequest *request,
                       ola::rdm::RDMCallback *on_complete,
                       ola::TimeStamp expiry_time,
                       IPV4Address &destination_ip,
                       uint8_t sequence_number)
        : request(request),
          on_complete(on_complete),
          expiry_time(expiry_time),
          destination_ip(destination_ip),
          sequence_number(sequence_number) {
    }

    const ola::rdm::RDMRequest *request;
    ola::rdm::RDMCallback *on_complete;
    ola::TimeStamp expiry_time;
    IPV4Address destination_ip;
    uint8_t sequence_number;
};


/**
 * A list of pending requests ordered by expiry time.
 */
class E133RequestContainer {
  public:
    E133RequestContainer() {}

    typedef std::deque<PendingE133Request>::iterator  request_iterator;

    void Add(PendingE133Request &request);
    bool IsEmpty() const { return m_requests.empty(); }
    const PendingE133Request &Front() const { return m_requests.front(); }
    void Erase(request_iterator iter) { m_requests.erase(iter); }
    request_iterator Begin() { return m_requests.begin(); }
    request_iterator End() { return m_requests.end(); }
    void PopFront();

  private:
    typedef std::deque<PendingE133Request> request_list;
    request_list m_requests;
};


/**
 * A RDM Controller for a single E1.33 universe
 */
class E133UniverseController: public ola::rdm::RDMControllerInterface,
                              public E133Component {
  public:
    explicit E133UniverseController(unsigned int universe);

    unsigned int Universe() const { return m_universe; }
    void SetE133Layer(E133Layer *e133_layer) { m_e133_layer = e133_layer; }

    // For now we use these to populate the uid map. This will get replaced by
    // the discovery code later.
    void AddUID(const UID &uid, const IPV4Address &target);
    void RemoveUID(const UID &uid);

    // Check for requests that need to be timed out
    void CheckForStaleRequests(const ola::TimeStamp *now);

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete);

    void HandlePacket(
        const ola::plugin::e131::TransportHeader &transport_header,
        const ola::plugin::e131::E133Header &e133_header,
        const std::string &raw_response);

  private:
    typedef struct {
      IPV4Address ip_address;
      uint8_t sequence_number;
    } uid_state;
    typedef std::map<UID, uid_state> uid_state_map;

    uid_state_map m_uid_state_map;
    E133Layer *m_e133_layer;
    unsigned int m_universe;
    E133RequestContainer m_requests;
    uid_state m_squawk_state;
    ola::Clock m_clock;

    bool PackRDMRequest(const ola::rdm::RDMRequest *request,
                        uint8_t **rdm_data,
                        unsigned int *rdm_data_size);
    void SendBroadcastRequest(const ola::rdm::RDMRequest *request,
                              ola::rdm::RDMCallback *on_complete);
    bool SendDataToUid(uid_state &uid_info,
                       const uint8_t *data,
                       unsigned int data_size);
    void SquawkRequest(const ola::rdm::RDMRequest *request);
    void SquawkResponse(const ola::rdm::RDMResponse *request);

    static const char UNIVERSE_SQUAWK_IP_ADDRESS[];
};
#endif  // TOOLS_E133_E133UNIVERSECONTROLLER_H_
