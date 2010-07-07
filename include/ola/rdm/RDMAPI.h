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
 * RDMAPI.h
 * Provide a generic RDM API that can use different implementations.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_RDMAPI_H_
#define INCLUDE_OLA_RDM_RDMAPI_H_

#include <stdint.h>
#include <ola/Callback.h>
#include <ola/rdm/RDMAPIImplInterface.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/UID.h>
#include <map>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

using std::string;
using std::vector;
using ola::SingleUseCallback1;
using ola::SingleUseCallback2;
using ola::SingleUseCallback4;


/*
 * Represents the state of a response (ack, nack etc.) and the reason if there
 * is one.
 *
 * RDM Handles should first check for Error() being non-empty as this
 * represents an underlying transport error. Then the value of response_type
 * should be checked against the rdm_response_type codes.
 */
class ResponseStatus {
  public:
    ResponseStatus(const RDMAPIImplResponseStatus &status,
                   const string &data);

    bool WasBroadcast() const { return m_was_broadcast; }
    bool WasNacked() const { return m_was_nacked; }
    uint16_t NackReason() const { return m_nack_reason; }
    const string& Error() const { return m_error; }

  private:
    bool m_was_broadcast;
    bool m_was_nacked;
    uint16_t m_nack_reason;  // The NACK reason if the type was NACK_REASON
    string m_error;  // Non empty if the RPC failed
};


/*
 * Represents a Status Message
 */
class StatusMessage {
  public:
    StatusMessage(uint16_t sub_device,
                  rdm_status_type status_type,
                  uint16_t status_message_id,
                  int16_t value1,
                  int16_t value2):
      sub_device(sub_device),
      status_type(status_type),
      status_message_id(status_message_id),
      value1(value1),
      value2(value2) {
    }

    uint16_t sub_device;
    rdm_status_type status_type;
    uint16_t status_message_id;
    int16_t value1;
    int16_t value2;
};


/*
 * Represents the description for a parameter
 */
class ParameterDescription {
  public:
    ParameterDescription() {}


    uint16_t m_pid;
    uint8_t m_pdl_size;
    rdm_data_type m_data_type;
    rdm_command_class m_command_class;
    rdm_pid_unit m_unit;
    rdm_pid_prefix m_prefix;
    uint32_t m_min_value;
    uint32_t m_default_value;
    uint32_t m_max_value;
    string m_description;
};

/*
 * Represents a DeviceInfo reply
 */
class DeviceInfo {
  public:

    uint16_t protocol_version;
    uint16_t device_model;
    rdm_product_category product_category;
};


/*
 * An object which deals with queued messages
 */
class QueuedMessageHandler {
  public:
    virtual ~QueuedMessageHandler() {}
};


/*
 * The high level RDM API.
 */
class RDMAPI {
  public:
    explicit RDMAPI(unsigned int universe,
                    class RDMAPIImplInterface *impl):
      m_universe(universe),
      m_impl(impl) {
    }
    ~RDMAPI() {}

    // This is used to check for queued messages
    uint8_t OutstandingMessagesCount(const UID &uid);

    // Network Managment Methods
    bool GetCommStatus(
        const UID &uid,
        SingleUseCallback4<void,
                           const ResponseStatus&,
                           uint16_t,
                           uint16_t,
                           uint16_t> *callback);

    bool ClearCommStatus(
        const UID &uid,
        SingleUseCallback1<void, const ResponseStatus&> *callback);

    bool GetQueuedMessage(
      const UID &uid,
      rdm_status_type status_type,
      QueuedMessageHandler *message_handler);

    bool GetStatusMessage(
      const UID &uid,
      rdm_status_type status_type,
      SingleUseCallback2<void,
                         const ResponseStatus&,
                         const vector<StatusMessage> > *callback);

    bool GetStatusIdDescription(
      const UID &uid,
      uint16_t status_id,
      SingleUseCallback2<void, const ResponseStatus&, const string&> *callback);

    bool ClearStatusId(
      const UID &uid,
      SingleUseCallback1<void, const ResponseStatus&> *callback);

    bool GetSubDeviceReporting(
      const UID &uid,
      uint16_t sub_device,
      SingleUseCallback2<void,
                         const ResponseStatus&,
                         rdm_status_type> *callback);

    bool SetSubDeviceReporting(
      const UID &uid,
      uint16_t sub_device,
      rdm_status_type status_type,
      SingleUseCallback1<void, const ResponseStatus&> *callback);

    // Information Methods
    bool GetSupportedParameters(
      const UID &uid,
      uint16_t sub_device,
      SingleUseCallback2<void,
                         const ResponseStatus&,
                         vector<uint16_t> > *callback);

    bool GetParameterDescription(
      const UID &uid,
      uint16_t pid,
      SingleUseCallback2<void,
                         const ResponseStatus&,
                         ParameterDescription> *callback);

    bool GetDeviceInfo(
      const UID &uid,
      uint16_t sub_device,
      SingleUseCallback2<void,
                         const ResponseStatus&,
                         const DeviceInfo&> *callback);




    // Handlers, these are called by the RDMAPIImpl.
    void HandleGetSupportedParameters(
      SingleUseCallback2<void,
                         const ResponseStatus&,
                         vector<uint16_t> > *callback,
      const RDMAPIImplResponseStatus &status,
      const string &data);


    void HandleGetDeviceInfo(
      SingleUseCallback2<void,
                         const ResponseStatus&,
                         const DeviceInfo&> *callback,
      const RDMAPIImplResponseStatus &status,
      const string &data);

  private:
    unsigned int m_universe;
    class RDMAPIImplInterface *m_impl;
    std::map<UID, uint8_t> m_outstanding_messages;
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_RDMAPI_H_
