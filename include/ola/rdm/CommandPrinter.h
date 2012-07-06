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
 * CommandPrinter.h
 * Write out RDMCommands in a human-readable format.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_COMMANDPRINTER_H_
#define INCLUDE_OLA_RDM_COMMANDPRINTER_H_

#include <stdint.h>
#include <ola/rdm/RDMEnums.h>
#include <string>

namespace ola {
namespace rdm {

using std::string;


class CommandPrinter {
  public:
    CommandPrinter(std::ostream *output,
                   class PidStoreHelper *pid_helper);
    ~CommandPrinter() {}

    void DisplayRequest(const class RDMRequest *request,
                        bool summarize = false,
                        bool unpack_param_data = true);
    void DisplayResponse(const class RDMResponse *response,
                         bool summarize = false,
                         bool unpack_param_data = true);

    void DisplayDiscoveryRequest(const class DiscoveryRequest *request,
                                 bool summarize = false,
                                 bool unpack_param_data = true);

    void DisplayDiscoveryResponse(const class DiscoveryResponse *response,
                                  bool summarize = false,
                                  bool unpack_param_data = true);

  private:
    std::ostream *m_output;
    PidStoreHelper *m_pid_helper;

    void AppendUIDsAndType(const class RDMCommand *command,
                           const char *message_type);
    void AppendPortId(const class RDMRequest *request);
    void AppendPortId(const class DiscoveryRequest *request);
    void AppendVerboseUIDs(const class RDMCommand *command);
    void AppendPidString(const class RDMCommand *command,
                         const class PidDescriptor *pid_descriptor);
    void AppendResponseType(const class RDMResponse *response);
    void AppendDiscoveryResponseType(const class DiscoveryResponse *response);
    void AppendVerboseResponseType(const class RDMResponse *response);
    void AppendVerboseDiscoveryResponseType(
        const class DiscoveryResponse *response);
    void AppendHeaderFields(const class RDMCommand *command,
                            const char *command_class);

    void DisplayParamData(
        const class PidDescriptor *pid_descriptor,
        bool unpack_param_data,
        bool is_request,
        bool is_get,
        const uint8_t *param_data,
        unsigned int param_length);
    bool GetNackReason(const class RDMCommand *command, uint16_t *reason);
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_COMMANDPRINTER_H_
