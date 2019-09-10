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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * CommandPrinter.cpp
 * Write out RDMCommands in a human-readable format.
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/StringUtils.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/CommandPrinter.h>
#include <ola/rdm/PidStore.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMCommand.h>
#include <iostream>
#include <memory>
#include <string>

namespace ola {
namespace rdm {

using ola::messaging::Descriptor;
using ola::messaging::Message;
using ola::network::NetworkToHost;
using ola::strings::ToHex;
using std::auto_ptr;
using std::endl;
using std::string;

/**
 * Constructor
 * @param output the ostream to write to.
 * @param pid_helper a pointer to a PidStoreHelper object
 */
CommandPrinter::CommandPrinter(std::ostream *output,
                               PidStoreHelper *pid_helper)
    : m_output(output),
      m_pid_helper(pid_helper) {
}


void CommandPrinter::Print(const class RDMCommand *,
                           bool,
                           bool) {
  *m_output << "Unknown RDM Command class";
}


void CommandPrinter::Print(const class RDMRequest *request,
                           bool summarize,
                           bool unpack_param_data) {
  DisplayRequest(request, summarize, unpack_param_data);
}


void CommandPrinter::Print(const class RDMResponse *response,
                           bool summarize,
                           bool unpack_param_data) {
  DisplayResponse(response, summarize, unpack_param_data);
}


void CommandPrinter::Print(const class RDMDiscoveryRequest *request,
                           bool summarize,
                           bool unpack_param_data) {
  DisplayDiscoveryRequest(request, summarize, unpack_param_data);
}


void CommandPrinter::Print(const class RDMDiscoveryResponse *response,
                           bool summarize,
                           bool unpack_param_data) {
  DisplayDiscoveryResponse(response, summarize, unpack_param_data);
}


/**
 * Write out a RDM Request
 * @param request the request to format
 * @param summarize enable the one line summary
 * @param unpack_param_data if the summary isn't enabled, this controls if we
 *   unpack and display parameter data.
 */
void CommandPrinter::DisplayRequest(const RDMRequest *request,
                                    bool summarize,
                                    bool unpack_param_data) {
  const PidDescriptor *descriptor = m_pid_helper->GetDescriptor(
       request->ParamId(),
       request->DestinationUID().ManufacturerId());
  bool is_get = request->CommandClass() == RDMCommand::GET_COMMAND;

  if (summarize) {
    AppendUIDsAndType(request, (is_get ? "GET" : "SET"));
    *m_output << ", port: " << std::dec << static_cast<int>(request->PortId())
      << ", ";
    AppendPidString(request, descriptor);
  } else {
    AppendVerboseUIDs(request);
    AppendPortId(request);
    AppendHeaderFields(request, (is_get ? "GET" : "SET"));

    *m_output << "  Param ID       : " << ToHex(request->ParamId());
    if (descriptor) {
      *m_output << " (" << descriptor->Name() << ")";
    }
    *m_output << endl;
    *m_output << "  Param data len : " << request->ParamDataSize() << endl;
    DisplayParamData(descriptor,
                     unpack_param_data,
                     true,
                     is_get,
                     request->ParamData(),
                     request->ParamDataSize());
  }
}


/**
 * Write out a RDM Response
 * @param response the response to format
 * @param summarize enable the one line summary
 * @param unpack_param_data if the summary isn't enabled, this controls if we
 *   unpack and display parameter data.
 */
void CommandPrinter::DisplayResponse(const RDMResponse *response,
                                     bool summarize,
                                     bool unpack_param_data) {
  const PidDescriptor *descriptor = m_pid_helper->GetDescriptor(
       response->ParamId(),
       response->SourceUID().ManufacturerId());

  bool is_get = response->CommandClass() == RDMCommand::GET_COMMAND_RESPONSE;

  if (summarize) {
    AppendUIDsAndType(response, (is_get ? "GET_RESPONSE" : "SET_RESPONSE"));
    *m_output << ", response type: ";
    AppendResponseType(response);
    *m_output << ", ";
    AppendPidString(response, descriptor);
  } else {
    AppendVerboseUIDs(response);
    AppendVerboseResponseType(response);
    AppendHeaderFields(response, (is_get ? "GET_RESPONSE" : "SET_RESPONSE"));

    *m_output << "  Param ID       : " << ToHex(response->ParamId());
    if (descriptor) {
      *m_output << " (" << descriptor->Name() << ")";
    }
    *m_output << endl;
    *m_output << "  Param data len : " << response->ParamDataSize()
      << endl;
    DisplayParamData(descriptor,
                     unpack_param_data,
                     false,
                     is_get,
                     response->ParamData(),
                     response->ParamDataSize());
  }
}


/**
 * Write out a RDM discovery request
 * @param request the request to format
 * @param summarize enable the one line summary
 * @param unpack_param_data if the summary isn't enabled, this controls if we
 *   unpack and display parameter data.
 */
void CommandPrinter::DisplayDiscoveryRequest(
    const RDMDiscoveryRequest *request,
    bool summarize,
    bool unpack_param_data) {
  // We can't just get a descriptor here like the other code as we don't store
  // them for discovery commands
  string param_name;
  switch (request->ParamId()) {
    case ola::rdm::PID_DISC_UNIQUE_BRANCH:
      param_name = "DISC_UNIQUE_BRANCH";
      break;
    case ola::rdm::PID_DISC_MUTE:
      param_name = "DISC_MUTE";
      break;
    case ola::rdm::PID_DISC_UN_MUTE:
      param_name = "DISC_UN_MUTE";
      break;
  }

  if (summarize) {
    AppendUIDsAndType(request, "DISCOVERY_COMMAND");
    *m_output << ", PID " << ToHex(request->ParamId());
    if (!param_name.empty())
      *m_output << " (" << param_name << ")";
    if (request->ParamId() == ola::rdm::PID_DISC_UNIQUE_BRANCH &&
        request->ParamDataSize() == 2 * UID::UID_SIZE) {
      const uint8_t *param_data = request->ParamData();
      UID lower(param_data);
      UID upper(param_data + UID::UID_SIZE);
      *m_output << ", (" << lower << ", " << upper << ")";
    } else {
      *m_output << ", PDL: " << std::dec << request->ParamDataSize();
    }
    *m_output << endl;
  } else {
    AppendVerboseUIDs(request);
    AppendPortId(request);
    AppendHeaderFields(request, "DISCOVERY_COMMAND");

    *m_output << "  Param ID       : " << ToHex(request->ParamId());
    if (!param_name.empty()) {
      *m_output << " (" << param_name << ")";
    }
    *m_output << endl;
    *m_output << "  Param data len : " << std::dec << request->ParamDataSize()
              << endl;
    if (request->ParamId() == ola::rdm::PID_DISC_UNIQUE_BRANCH &&
        request->ParamDataSize() == 2 * UID::UID_SIZE) {
      const uint8_t *param_data = request->ParamData();
      UID lower(param_data);
      UID upper(param_data + UID::UID_SIZE);
      *m_output << "  Lower UID      : " << lower << endl;
      *m_output << "  Upper UID      : " << upper << endl;
    } else {
      DisplayParamData(NULL,
                       unpack_param_data,
                       true,
                       false,
                       request->ParamData(),
                       request->ParamDataSize());
    }
  }
}


/**
 * Write out a RDM discovery response.
 * @param response the response to format.
 * @param summarize enable the one line summary
 * @param unpack_param_data if the summary isn't enabled, this controls if we
 *   unpack and display parameter data.
 */
void CommandPrinter::DisplayDiscoveryResponse(
    const RDMDiscoveryResponse *response,
    bool summarize,
    bool unpack_param_data) {
  // We can't just get a descriptor here like the other code as we don't store
  // them for discovery commands
  string param_name;
  switch (response->ParamId()) {
    case ola::rdm::PID_DISC_UNIQUE_BRANCH:
      param_name = "DISC_UNIQUE_BRANCH";
      break;
    case ola::rdm::PID_DISC_MUTE:
      param_name = "DISC_MUTE";
      break;
    case ola::rdm::PID_DISC_UN_MUTE:
      param_name = "DISC_UN_MUTE";
      break;
  }

  if (summarize) {
    AppendUIDsAndType(response, "DISCOVERY_COMMAND_RESPONSE");
    *m_output << ", PID " << ToHex(response->ParamId());
    if (!param_name.empty()) {
      *m_output << " (" << param_name << ")";
    }
    if (response->ParamId() == ola::rdm::PID_DISC_UNIQUE_BRANCH &&
        response->ParamDataSize() == 2 * UID::UID_SIZE) {
      const uint8_t *param_data = response->ParamData();
      UID lower(param_data);
      UID upper(param_data + UID::UID_SIZE);
      *m_output << ", (" << lower << ", " << upper << ")";
    } else {
      *m_output << ", PDL: " << response->ParamDataSize();
    }
    *m_output << endl;
  } else {
    AppendVerboseUIDs(response);
    AppendVerboseResponseType(response);
    AppendHeaderFields(response, "DISCOVERY_COMMAND_RESPONSE");

    *m_output << "  Param ID       : " << ToHex(response->ParamId());
    if (!param_name.empty()) {
      *m_output << " (" << param_name << ")";
    }
    *m_output << endl;
    *m_output << "  Param data len : " << response->ParamDataSize()
              << endl;
    DisplayParamData(NULL,
                     unpack_param_data,
                     true,
                     false,
                     response->ParamData(),
                     response->ParamDataSize());
  }
}


/**
 * Append the src/dst UIDs & type
 */
void CommandPrinter::AppendUIDsAndType(const class RDMCommand *command,
                                       const char *message_type) {
  *m_output <<
    command->SourceUID() << " -> " << command->DestinationUID() << " " <<
    message_type << ", Sub-Device: " << std::dec << command->SubDevice() <<
    ", TN: " << static_cast<int>(command->TransactionNumber());
}


void CommandPrinter::AppendPortId(const class RDMRequest *request) {
  *m_output << "  Port ID        : " << std::dec <<
    static_cast<unsigned int>(request->PortId()) << endl;
}


void CommandPrinter::AppendMessageLength(const class RDMRequest *request) {
  *m_output << "  Message Length : " << std::dec <<
    static_cast<unsigned int>(request->MessageLength()) << endl;
}


void CommandPrinter::AppendVerboseUIDs(const class RDMCommand *command) {
  *m_output << "  Source UID     : " << command->SourceUID() << endl;
  *m_output << "  Dest UID       : " << command->DestinationUID() << endl;
  *m_output << "  Transaction #  : " << std::dec <<
    static_cast<unsigned int>(command->TransactionNumber()) << endl;
}


void CommandPrinter::AppendResponseType(const RDMResponse *response) {
  switch (response->ResponseType()) {
    case ola::rdm::RDM_ACK:
      *m_output << "ACK";
      break;
    case ola::rdm::RDM_ACK_TIMER:
      *m_output << "ACK TIMER";
      break;
    case ola::rdm::RDM_NACK_REASON:
      uint16_t reason;
      if (GetNackReason(response, &reason)) {
        *m_output << "NACK (" << ola::rdm::NackReasonToString(reason) << ")";
      } else {
        *m_output << "Malformed NACK ";
      }
      break;
    case ola::rdm::ACK_OVERFLOW:
      *m_output << "ACK OVERFLOW";
      break;
    default:
      *m_output << "Unknown (" << response->ResponseType() << ")";
  }
}


void CommandPrinter::AppendVerboseResponseType(
    const RDMResponse *response) {
  *m_output << "  Response Type  : ";
  AppendResponseType(response);
  *m_output  << endl;
}


void CommandPrinter::AppendHeaderFields(
    const RDMCommand *command,
    const char *command_class) {
  *m_output << "  Message count  : "
            << static_cast<unsigned int>(command->MessageCount()) << endl;
  *m_output << "  Sub device     : " << command->SubDevice() << endl;
  *m_output << "  Message length : "
            << static_cast<unsigned int>(command->MessageLength()) << endl;
  *m_output << "  Command class  : " << command_class << endl;
}


/**
 * Append the PID descriptor
 */
void CommandPrinter::AppendPidString(const RDMCommand *command,
                                     const PidDescriptor *descriptor) {
  *m_output << "PID " << ToHex(command->ParamId());
  if (descriptor) {
    *m_output << " (" << descriptor->Name() << ")";
  }
  *m_output << ", PDL: " << command->ParamDataSize() << endl;
}


/**
 * Format parameter data.
 */
void CommandPrinter::DisplayParamData(
    const PidDescriptor *pid_descriptor,
    bool unpack_param_data,
    bool is_request,
    bool is_get,
    const uint8_t *param_data,
    unsigned int data_length) {
  if (!data_length) {
    return;
  }

  *m_output << "  Param data:" << endl;
  if (unpack_param_data && pid_descriptor) {
    const Descriptor *descriptor = NULL;
    if (is_request) {
      descriptor = (is_get ?
          pid_descriptor->GetRequest() : pid_descriptor->SetRequest());
    } else {
      descriptor = (is_get ?
         pid_descriptor->GetResponse() : pid_descriptor->SetResponse());
    }

    if (descriptor) {
      auto_ptr<const Message> message(
        m_pid_helper->DeserializeMessage(descriptor, param_data, data_length));

      if (message.get()) {
        *m_output << m_pid_helper->MessageToString(message.get());
        return;
      }
    }
  }

  // otherwise just display the raw data, indent 4, 8 bytes per line
  ola::FormatData(m_output, param_data, data_length, 4, 8);
}


/**
 * Get the nack reason.
 */
bool CommandPrinter::GetNackReason(const RDMCommand *response,
                                   uint16_t *reason) {
  if (response->ParamDataSize() == 2) {
    memcpy(reinterpret_cast<uint8_t*>(reason),
           response->ParamData(),
           sizeof(*reason));
    *reason = NetworkToHost(*reason);
    return true;
  } else {
    return false;
  }
}
}  // namespace rdm
}  // namespace ola
