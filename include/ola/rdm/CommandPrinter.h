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

    void DisplayRequest(uint8_t sub_start_code,
                        uint8_t message_length,
                        const class RDMRequest *request,
                        bool summarize = false,
                        bool unpack_param_data = true);
    void DisplayResponse(uint8_t sub_start_code,
                         uint8_t message_length,
                         const class RDMResponse *response,
                         bool summarize = false,
                         bool unpack_param_data = true);

    void DisplayDiscovery(uint8_t sub_start_code,
                          uint8_t message_length,
                          const class RDMDiscoveryCommand *command,
                          bool summarize = false,
                          bool unpack_param_data = true);

    void DisplayRawData(const uint8_t *data, unsigned int length);

  private:
    std::ostream *m_output;
    PidStoreHelper *m_pid_helper;

    void DisplayParamData(
        const class PidDescriptor *pid_descriptor,
        bool unpack_param_data,
        bool is_get,
        const uint8_t *param_data,
        unsigned int param_length);
    bool GetNackReason(const class RDMResponse *response, uint16_t *reason);

    static const uint8_t BYTES_PER_LINE = 8;
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_COMMANDPRINTER_H_
