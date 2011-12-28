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
 * QueueingRDMController.h
 * A RDM Controller that sends a single message at a time.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_QUEUEINGRDMCONTROLLER_H_
#define INCLUDE_OLA_RDM_QUEUEINGRDMCONTROLLER_H_

#include <ola/rdm/RDMControllerInterface.h>
#include <queue>
#include <string>
#include <utility>
#include <vector>

namespace ola {
namespace rdm {

using std::pair;
using std::vector;

/*
 * A RDM controller that only sends a single request at a time. This also
 * handles timing out messages that we don't get a response for.
 */
class QueueingRDMController: public RDMControllerInterface {
  public:
    QueueingRDMController(RDMControllerInterface *controller,
                          unsigned int max_queue_size);
    ~QueueingRDMController();

    void Pause();
    void Resume();

    // This can be called multiple times and the requests will be queued.
    void SendRDMRequest(const RDMRequest *request, RDMCallback *on_complete);

  protected:
    typedef struct {
      const RDMRequest *request;
      RDMCallback *on_complete;
    } outstanding_rdm_request;

    RDMControllerInterface *m_controller;
    unsigned int m_max_queue_size;
    std::queue<outstanding_rdm_request> m_pending_requests;
    bool m_rdm_request_pending;  // true if a request is in progress
    bool m_active;  // true if the controller is active
    RDMCallback *m_callback;
    const ola::rdm::RDMResponse *m_response;
    vector<std::string> m_packets;

    virtual void TakeNextAction();
    virtual bool CheckForBlockingCondition();
    void MaybeSendRDMRequest();
    void DispatchNextRequest();

    void HandleRDMResponse(rdm_response_code status,
                           const ola::rdm::RDMResponse *response,
                           const vector<std::string> &packets);
};


/**
 * The DiscoverableQueueingRDMController also handles discovery, and ensures
 * that only a single discovery or RDM request sequence occurs at once.
 *
 * In this model discovery has a higher precedence that RDM messages.
 */
class DiscoverableQueueingRDMController: public QueueingRDMController {
  public:
    DiscoverableQueueingRDMController(
        DiscoverableRDMControllerInterface *controller,
        unsigned int max_queue_size);

    ~DiscoverableQueueingRDMController() {}

    // These can be called multiple times and the requests will be queued
    void RunFullDiscovery(RDMDiscoveryCallback *callback);
    void RunIncrementalDiscovery(RDMDiscoveryCallback *callback);

  private:
    typedef vector<RDMDiscoveryCallback*> DiscoveryCallbacks;
    typedef vector<pair<bool, RDMDiscoveryCallback*> >
        PendingDiscoveryCallbacks;

    DiscoverableRDMControllerInterface *m_discoverable_controller;
    DiscoveryCallbacks m_discovery_callbacks;
    PendingDiscoveryCallbacks m_pending_discovery_callbacks;

    void TakeNextAction();
    bool CheckForBlockingCondition();
    void GenericDiscovery(RDMDiscoveryCallback *callback, bool full);
    void StartRDMDiscovery();
    void DiscoveryComplete(const ola::rdm::UIDSet &uids);
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_QUEUEINGRDMCONTROLLER_H_
