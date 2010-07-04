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
#include <ola/rdm/RDMEnums.h>
#include <map>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

using std::string;
using std::vector;


/*
 * Represents the state of a response (ack, nack etc.) and the reason if there
 * is one.
 */
class ResponseStatus {
  public:
    ResponseStatus(rdm_response_type type, rdm_nack_reason reason):
      response_type(type),
      nack_reason(reason) {
    }

    rdm_response_type response_type;
    rdm_nack_reason nack_reason;
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
      m_sub_device(sub_device),
      m_status_type(status_type),
      m_status_message_id(status_message_id),
      m_value1(value1),
      m_value2(value2) {
  }


  private:
    uint16_t m_sub_device,
    rdm_status_type m_status_type,
    uint16_t m_status_message_id,
    int16_t m_value1,
    int16_t m_value2
};


/*
 * Represents the description for a parameter
 */
class ParameterDescription {
  public:
    ParameterDescription() {}


  private:
    uint16_t m_pid,
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
 * This is the interface for an RDMAPI implementation
 */
class RDMAPIImplInterface {
  public:
    virtual ~RDMAPIImplInterface() {}

    typedef ola::Callback2<void,
                           rdm_response_type,
                           const string&> rdm_callback;

    virtual bool RDMGet(rdm_callback *callback,
                        unsigned int universe,
                        const UID &uid,
                        uint16_t sub_device,
                        uint16_t pid,
                        const uint8_t *data = NULL,
                        unsigned int data_length = 0) = 0;
    virtual bool RDMSet(rdm_callback *callback,
                        unsigned int universe,
                        const UID &uid,
                        uint16_t sub_device,
                        uint16_t pid,
                        const uint8_t *data,
                        unsigned int data_length) = 0;
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
    explicit RDMAPI(RDMAPIImplInterface *impl):
      m_impl(impl) {
    }
    ~RDMAPI() {}

    // This is used to check for queued messages
    uint8_t OutstandingMessagesCount(const UID &uid);

    // Network Managment Methods
    bool GetCommStatus(
        const UID &uid,
        ola::Callback<void,
                      ResponseStatus response_type,
                      uint16_t short_message,
                      uint16_t length_mismatch,
                      uint16_t checksum_fail> *callback);

    bool ClearCommStatus(
        const UID &uid,
        ola::Callback<void, ResponseStatus> *callback);

    bool GetQueuedMessage(
      const UID &uid,
      rdm_status_type status_type,
      QueuedMessageHandler *message_handler);

    bool GetStatusMessage(
      const UID &uid,
      rdm_status_type status_type,
      ola::Callback<void,
                    ResponseStatus response_status,
                    const vector<StatusMessage> *callback);

    bool GetStatusIdDescription(
      const UID &uid,
      uint16_t status_id,
      ola::Callback<void,
                    ResponseStatus response_status,
                    const string &description> *callback);

    bool ClearStatusId(
      const UID &uid,
      ola::Callback<void, ResponseStatus response_status> *callback);

    bool GetSubDeviceReporting(
      const UID &uid,
      uint16_t sub_device,
      ola::Callback<void,
                    ResponseStatus response_status,
                    rdm_status_type> *callback);

    bool SetSubDeviceReporting(
      const UID &uid,
      uint16_t sub_device,
      rdm_status_type status_type,
      ola::Callback<void, ResponseStatus response_status> *callback);

    // Information Methods
    bool GetSupportedParameters(
      const UID &uid,
      uint16_t sub_device,
      ola::Callback<void,
                    ResponseStatus response_status,
                    vector<uint16_t> > *callback);

    bool GetParameterDescription(
      const UID &uid,
      uint16_t pid,
      ola::Callback<void,
                    ResponseStatus response_status,
                    ParameterDescription> *callback);

    bool GetDeviceInfo(
      const UID &uid,
      uint16_t sub_device,
      ola::Callback<void,
                    ResponseStatus response_status,
                    DeviceInfo> *callback);


  private:
    RDMAPIImplInterface *m_impl;
    std::map<UID, uint8_t> m_outstanding_messages;
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_RDMAPI_H_
