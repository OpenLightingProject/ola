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
 * RDMAPIImplInterface.h
 * The interface for an RDM API Implementation
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_RDMAPIIMPLINTERFACE_H_
#define INCLUDE_OLA_RDM_RDMAPIIMPLINTERFACE_H_

#include <stdint.h>
#include <ola/rdm/UID.h>
#include <ola/Callback.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <string>

namespace ola {
namespace rdm {

using std::string;


/*
 * Represents the state of a response and/or any error codes.
 *
 * RDM Handlers should first check for error being non-empty as this
 * represents an underlying transport error. Then the resonse_code
 * should be checked to catch invalid responses, timeouts etc. Finally, the
 * value of response_type should be checked against the rdm_response_type
 * codes.
 */
class ResponseStatus {
  public:
    string error;  // Non empty if the RPC failed
    rdm_response_code response_code;
    uint8_t response_type;  // The RDM response type
    uint8_t message_count;  // Number of queued messages
    uint16_t m_param;

    // helper methods
    bool WasAcked() const {
      return (error.empty() && response_code == RDM_COMPLETED_OK &&
          response_type == RDM_ACK);
    }

    bool WasNacked() const {
      return (error.empty() && response_code == RDM_COMPLETED_OK &&
          response_type == RDM_NACK_REASON);
    }

    // Returns the NACK Reason code
    uint16_t NackReason() const { return m_param; }

    // Returns the time (in ms) to wait before re-trying
    unsigned int AckTimer() const { return 100 * m_param; }
};


/*
 * This is the interface for an RDMAPI implementation
 */
class RDMAPIImplInterface {
  public:
    virtual ~RDMAPIImplInterface() {}

    // args are the response type the param data
    typedef ola::SingleUseCallback2<void,
                                    const ResponseStatus&,
                                    const string&> rdm_callback;

    // args are response type, pid & param data
    typedef ola::SingleUseCallback3<void,
                                    const ResponseStatus&,
                                    uint16_t,
                                    const string&> rdm_pid_callback;

    // get command
    virtual bool RDMGet(rdm_callback *callback,
                        unsigned int universe,
                        const UID &uid,
                        uint16_t sub_device,
                        uint16_t pid,
                        const uint8_t *data = NULL,
                        unsigned int data_length = 0) = 0;

    // A version of Get that also returns the pid. This is used to deal with
    // queued messages
    virtual bool RDMGet(rdm_pid_callback *callback,
                        unsigned int universe,
                        const UID &uid,
                        uint16_t sub_device,
                        uint16_t pid,
                        const uint8_t *data = NULL,
                        unsigned int data_length = 0) = 0;

    // set command
    virtual bool RDMSet(rdm_callback *callback,
                        unsigned int universe,
                        const UID &uid,
                        uint16_t sub_device,
                        uint16_t pid,
                        const uint8_t *data = NULL,
                        unsigned int data_length = 0) = 0;
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_RDMAPIIMPLINTERFACE_H_
