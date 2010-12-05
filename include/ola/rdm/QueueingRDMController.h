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
    void SendRDMRequest(const RDMRequest *request,
                        RDMCallback *on_complete);

  private:
    typedef struct {
      const RDMRequest *request;
      RDMCallback *on_complete;
    } outstanding_rdm_request;

    RDMControllerInterface *m_controller;
    unsigned int m_max_queue_size;
    std::queue<outstanding_rdm_request> m_pending_requests;
    bool m_rdm_request_pending;
    bool m_active;
    RDMCallback *m_callback;
    const ola::rdm::RDMResponse *m_response;

    void MaybeSendRDMRequest();
    void DispatchNextRequest();

    void HandleRDMResponse(rdm_request_status status,
                           const ola::rdm::RDMResponse *response);
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_QUEUEINGRDMCONTROLLER_H_
