/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ClientRDMAPIShim.h
 * An implemention of RDMAPIImplInterface that uses the OlaClient.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_CLIENT_CLIENTRDMAPISHIM_H_
#define INCLUDE_OLA_CLIENT_CLIENTRDMAPISHIM_H_

#include <ola/client/ClientTypes.h>
#include <ola/client/Result.h>
#include <ola/rdm/RDMAPIImplInterface.h>
#include <ola/rdm/RDMCommand.h>

#include <string>

namespace ola {
namespace client {

class OlaClient;

/**
 * An implemention of RDMAPIImplInterface that uses the OlaClient.
 */
class ClientRDMAPIShim : public ola::rdm::RDMAPIImplInterface {
 public:
    explicit ClientRDMAPIShim(OlaClient *client)
        : m_client(client) {
    }

    bool RDMGet(rdm_callback *callback,
                unsigned int universe,
                const ola::rdm::UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data = NULL,
                unsigned int data_length = 0);

    bool RDMGet(rdm_pid_callback *callback,
                unsigned int universe,
                const ola::rdm::UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data = NULL,
                unsigned int data_length = 0);

    bool RDMSet(rdm_callback *callback,
                unsigned int universe,
                const ola::rdm::UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data = NULL,
                unsigned int data_length = 0);

 private:
    OlaClient *m_client;

    void HandleResponse(
        rdm_callback *callback,
        const Result &result,
        const RDMMetadata &metadata,
        const ola::rdm::RDMResponse *response);

    void HandleResponseWithPid(
        rdm_pid_callback *callback,
        const Result &result,
        const RDMMetadata &metadata,
        const ola::rdm::RDMResponse *response);

    void GetResponseStatusAndData(
        const Result &result,
        ola::rdm::rdm_response_code response_code,
        const ola::rdm::RDMResponse *response,
        rdm::ResponseStatus *response_status,
        string *data);

    void GetParamFromReply(const std::string &message_type,
                           const ola::rdm::RDMResponse *reply,
                           ola::rdm::ResponseStatus *new_status);
};
}  // namespace client
}  // namespace ola
#endif  // INCLUDE_OLA_CLIENT_CLIENTRDMAPISHIM_H_
