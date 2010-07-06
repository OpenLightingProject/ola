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
 * InternalRDMController.h
 * Manager internally generated RDM requests
 * Copyright (C) 2010 Simon Newton
 */

#ifndef OLAD_INTERNALRDMCONTROLLER_H_
#define OLAD_INTERNALRDMCONTROLLER_H_

#include <map>
#include <string>
#include <vector>

#include "ola/Callback.h"
#include "ola/Clock.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/UID.h"
#include "olad/InternalInputPort.h"
#include "olad/PortManager.h"

namespace ola {

using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;

typedef ola::SingleUseCallback1<void, const RDMResponse*>
  rdm_controller_callback;

/*
 * This represents an RDMRequest that we haven't got a response for yet
 */
class OutstandingRDMRequest {
  public:
    OutstandingRDMRequest(const RDMRequest &request,
                          rdm_controller_callback *callback);
    bool Matches(const RDMResponse *response);
    bool HasExpired(const TimeStamp &now);
    void RunCallback(const RDMResponse *response);

  private:
    OutstandingRDMRequest(const OutstandingRDMRequest&);
    OutstandingRDMRequest& operator=(const OutstandingRDMRequest&);

    const UID m_source_uid;
    uint8_t m_sub_device;
    uint8_t m_transaction_number;
    ola::rdm::RDMCommand::RDMCommandClass m_command_class;
    TimeStamp m_expires;
    rdm_controller_callback *m_callback;
};


/*
 * This class manages RDM requests generated internally.
 */
class InternalRDMController: public InternalInputPortResponseHandler {
  public:
    InternalRDMController(const UID &default_uid,
                          PortManager *port_manager,
                          class ExportMap *export_map);
    ~InternalRDMController();

    bool SendRDMRequest(
        Universe *universe,
        const UID &destination,
        uint8_t sub_device,
        uint16_t param_id,
        const string &data,
        bool is_set,
        rdm_controller_callback *callback,
        const UID *source = NULL);

    bool HandleRDMResponse(unsigned int universe,
                           const ola::rdm::RDMResponse *response);
    void CheckTimeouts(const TimeStamp &now);

  private :
    InternalRDMController(const InternalRDMController&);
    InternalRDMController& operator=(const InternalRDMController&);

    const UID m_default_uid;
    PortManager *m_port_manager;
    map<unsigned int, InternalInputPort*> m_input_ports;
    map<pair<unsigned int, const UID>, uint8_t> m_transaction_numbers;
    map<unsigned int, vector<OutstandingRDMRequest*> > m_outstanding_requests;
    class ExportMap *m_export_map;

    static const char MISMATCHED_RDM_RESPONSE_VAR[];
    static const char EXPIRED_RDM_REQUESTS_VAR[];
};
}  // ola
#endif  // OLAD_INTERNALRDMCONTROLLER_H_
