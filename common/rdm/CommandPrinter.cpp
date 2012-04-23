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

#include <ola/rdm/CommandPrinter.h>
#include <ola/rdm/PidStore.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/network/NetworkUtils.h>
#include <iostream>
#include <memory>
#include <string>

namespace ola {
namespace rdm {

using ola::messaging::Message;
using ola::network::NetworkToHost;
using std::auto_ptr;
using std::endl;
using std::stringstream;

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


/**
 * Write out a RDM Request
 * @param start_code the start code for this message
 * @param message_length the length of the RDM message
 * @param request the request to format
 * @param summarize enable the one line summary
 * @param unpack_param_data if the summary isn't enabled, this controls if we
 *   unpack and display parameter data.
 */
void CommandPrinter::DisplayRequest(uint8_t start_code,
                                    uint8_t message_length,
                                    const RDMRequest *request,
                                    bool summarize,
                                    bool unpack_param_data) {
  const PidDescriptor *descriptor = m_pid_helper->GetDescriptor(
       request->ParamId(),
       request->DestinationUID().ManufacturerId());
  bool is_get = request->CommandClass() == RDMCommand::GET_COMMAND;

  if (summarize) {
    *m_output <<
      request->SourceUID() << " -> " << request->DestinationUID() << " " <<
      (is_get ? "GET" : "SET") <<
      ", sub-device: " << std::dec << request->SubDevice() <<
      ", tn: " << static_cast<int>(request->TransactionNumber()) <<
      ", port: " << std::dec << static_cast<int>(request->PortId()) <<
      ", PID 0x" << std::hex << std::setfill('0') << std::setw(4) <<
        request->ParamId();
      if (descriptor)
        *m_output << " (" << descriptor->Name() << ")";
      *m_output << ", pdl: " << std::dec << request->ParamDataSize() << endl;
  } else {
    *m_output << "  Sub start code : 0x" << std::hex <<
      static_cast<unsigned int>(start_code) << endl;
    *m_output << "  Message length : " <<
      static_cast<unsigned int>(message_length) << endl;
    *m_output << "  Dest UID       : " << request->DestinationUID() << endl;
    *m_output << "  Source UID     : " << request->SourceUID() << endl;
    *m_output << "  Transaction #  : " << std::dec <<
      static_cast<unsigned int>(request->TransactionNumber()) << endl;
    *m_output << "  Port ID        : " << std::dec <<
      static_cast<unsigned int>(request->PortId()) << endl;
    *m_output << "  Message count  : " << std::dec <<
      static_cast<unsigned int>(request->MessageCount()) << endl;
    *m_output << "  Sub device     : " << std::dec << request->SubDevice()
      << endl;
    *m_output << "  Command class  : " << (is_get ? "GET" : "SET") << endl;
    *m_output << "  Param ID       : 0x" << std::setfill('0') << std::setw(4)
      << std::hex << request->ParamId();
    if (descriptor)
      *m_output << " (" << descriptor->Name() << ")";
    *m_output << endl;
    *m_output << "  Param data len : " << std::dec << request->ParamDataSize()
      << endl;
    DisplayParamData(descriptor,
                     unpack_param_data,
                     is_get,
                     request->ParamData(),
                     request->ParamDataSize());
  }
}


/**
 * Write out a RDM Response
 * @param start_code the start code for this message
 * @param message_length the length of the RDM message
 * @param response the response to format
 * @param summarize enable the one line summary
 * @param unpack_param_data if the summary isn't enabled, this controls if we
 *   unpack and display parameter data.
 */
void CommandPrinter::DisplayResponse(uint8_t start_code,
                                     uint8_t message_length,
                                     const RDMResponse *response,
                                     bool summarize,
                                     bool unpack_param_data) {
  const PidDescriptor *descriptor = m_pid_helper->GetDescriptor(
       response->ParamId(),
       response->SourceUID().ManufacturerId());

  bool is_get = response->CommandClass() == RDMCommand::GET_COMMAND_RESPONSE;

  if (summarize) {
    *m_output <<
      response->SourceUID() << " -> " << response->DestinationUID() << " " <<
      (is_get ? "GET_RESPONSE" : "SET_RESPONSE") <<
      ", sub-device: " << std::dec << response->SubDevice() <<
      ", tn: " << static_cast<int>(response->TransactionNumber()) <<
      ", response type: ";

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
    *m_output << ", PID 0x" << std::hex <<
      std::setfill('0') << std::setw(4) << response->ParamId();
    if (descriptor)
      *m_output << " (" << descriptor->Name() << ")";
    *m_output << ", pdl: " << std::dec << response->ParamDataSize() << endl;
  } else {
    *m_output << "  Sub start code : 0x" << std::hex <<
      static_cast<unsigned int>(start_code) << endl;
    *m_output << "  Message length : " <<
      static_cast<unsigned int>(message_length) << endl;
    *m_output << "  Dest UID       : " << response->DestinationUID() << endl;
    *m_output << "  Source UID     : " << response->SourceUID() << endl;
    *m_output << "  Transaction #  : " << std::dec <<
      static_cast<unsigned int>(response->TransactionNumber()) << endl;
    *m_output << "  Response Type  : ";
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
    *m_output  << endl;
    *m_output << "  Message count  : " << std::dec <<
      static_cast<unsigned int>(response->MessageCount()) << endl;
    *m_output << "  Sub device     : " << std::dec << response->SubDevice()
      << endl;
    *m_output << "  Command class  : " <<
      (is_get ? "GET_RESPONSE" : "SET_RESPONSE") << endl;

    *m_output << "  Param ID       : 0x" << std::setfill('0') << std::setw(4)
      << std::hex << response->ParamId();
    if (descriptor)
      *m_output << " (" << descriptor->Name() << ")";
    *m_output << endl;
    *m_output << "  Param data len : " << std::dec << response->ParamDataSize()
      << endl;
    DisplayParamData(descriptor,
                     unpack_param_data,
                     is_get,
                     response->ParamData(),
                     response->ParamDataSize());
  }
}


/**
 * Display raw data
 */
void CommandPrinter::DisplayRawData(const uint8_t *data, unsigned int length) {
  stringstream raw;
  raw << std::setw(2) << std::hex;
  stringstream ascii;
  for (unsigned int i = 0; i != length; i++) {
    raw << std::setw(2) << std::setfill('0') <<
      static_cast<unsigned int>(data[i]) << " ";
    if (data[i] >= ' ' && data[i] <= '~')
      ascii << data[i];
    else
      ascii << ".";

    if (i % BYTES_PER_LINE == BYTES_PER_LINE - 1) {
      *m_output << "    " << raw.str() << " " << ascii.str() << endl;
      raw.str("");
      ascii.str("");
    }
  }
  if (length % BYTES_PER_LINE != 0) {
    // pad if needed
    raw << string(3 * (BYTES_PER_LINE - (length % BYTES_PER_LINE)), ' ');
    *m_output << "    " << raw.str() << " " << ascii.str() << endl;
  }
}


/**
 * Format parameter data.
 */
void CommandPrinter::DisplayParamData(
    const PidDescriptor *pid_descriptor,
    bool unpack_param_data,
    bool is_get,
    const uint8_t *param_data,
    unsigned int data_length) {
  if (!data_length)
    return;

  *m_output << "  Param data:" << endl;
  if (unpack_param_data && pid_descriptor) {
    const Descriptor *descriptor =
        is_get ? pid_descriptor->GetResponse() : pid_descriptor->SetResponse();

    if (descriptor) {
      auto_ptr<const Message> message(
        m_pid_helper->DeserializeMessage(descriptor, param_data, data_length));

      if (message.get()) {
        *m_output << m_pid_helper->MessageToString(message.get());
        return;
      }
    }
  }

  // otherwise just display the raw data
  DisplayRawData(param_data, data_length);
}


/**
 * Get the nack reason.
 */
bool CommandPrinter::GetNackReason(const RDMResponse *response,
                                   uint16_t *reason) {
  if (response->ParamDataSize() != 2) {
    memcpy(reinterpret_cast<uint8_t*>(reason),
           response->ParamData(),
           sizeof(*reason));
    *reason = NetworkToHost(*reason);
    return true;
  } else {
    return false;
  }
}
}  // rdm
}  // ola
