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
#include <vector>

namespace ola {
namespace rdm {


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
    std::vector<std::string> m_packets;

    void MaybeSendRDMRequest();
    void DispatchNextRequest();
    virtual bool CheckForBlockingCondition();

    void HandleRDMResponse(rdm_response_code status,
                           const ola::rdm::RDMResponse *response,
                           const std::vector<std::string> &packets);
};


class DiscoverableQueueingRDMController: public QueueingRDMController {
  public:
    DiscoverableQueueingRDMController(
        DiscoverableRDMControllerInterface *controller,
        unsigned int max_queue_size);

    ~DiscoverableQueueingRDMController() {}

    bool RunFullDiscovery(RDMDiscoveryCallback *callback);
    bool RunIncrementalDiscovery(RDMDiscoveryCallback *callback);

  private:
    typedef enum {
      FREE,
      PENDING,
      RUNNING,
    } discovery_state;

    DiscoverableRDMControllerInterface *m_discoverable_controller;
    discovery_state m_discovery_state;
    RDMDiscoveryCallback *m_discovery_callback;
    bool m_full_discovery;

    bool GenericDiscovery(RDMDiscoveryCallback *callback, bool full);
    bool CheckForBlockingCondition();
    void StartRDMDiscovery();
    void DiscoveryComplete(const ola::rdm::UIDSet &uids);
    void RunCallback(const ola::rdm::UIDSet &uids);
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_QUEUEINGRDMCONTROLLER_H_
