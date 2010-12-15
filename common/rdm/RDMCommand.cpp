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
 * RDMCommand.cpp
 * The RDMCommand class
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <string>
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMCommand.h"

namespace ola {
namespace rdm {

/*
 * Constructor
 */
RDMCommand::RDMCommand(const UID &source,
                       const UID &destination,
                       uint8_t transaction_number,
                       uint8_t port_id,
                       uint8_t message_count,
                       uint16_t sub_device,
                       uint16_t param_id,
                       const uint8_t *data,
                       unsigned int length):
    m_port_id(port_id),
    m_source(source),
    m_destination(destination),
    m_transaction_number(transaction_number),
    m_message_count(message_count),
    m_sub_device(sub_device),
    m_param_id(param_id),
    m_data(NULL),
    m_data_length(length) {
  if (length > MAX_PARAM_DATA_LENGTH) {
    OLA_WARN << "Attempt to create RDM message with a length > " <<
      MAX_PARAM_DATA_LENGTH << ", was; " << length;
    m_data_length = MAX_PARAM_DATA_LENGTH;
  }

  if (m_data_length > 0 && data != NULL) {
    m_data = new uint8_t[m_data_length];
    memcpy(m_data, data, m_data_length);
  }
}


/*
 * Destructor
 */
RDMCommand::~RDMCommand() {
  if (m_data)
    delete[] m_data;
}


/*
 * Equality test
 */
bool RDMCommand::operator==(const RDMCommand &other) const {
  if (m_source == other.m_source &&
      m_destination == other.m_destination &&
      m_transaction_number == other.m_transaction_number &&
      m_message_count == other.m_message_count &&
      m_sub_device == other.m_sub_device &&
      CommandClass() == other.CommandClass() &&
      m_param_id == other.m_param_id &&
      m_data_length == other.m_data_length) {
    return 0 == memcmp(m_data, other.m_data, m_data_length);
  }
  return false;
}


std::string RDMCommand::ToString() const {
  std::stringstream str;
  str << m_source << " -> " << m_destination << ", Trans # " <<
    static_cast<int>(m_transaction_number) << ", Port ID " <<
    static_cast<int>(m_port_id) << ", Msg Cnt " <<
    static_cast<int>(m_message_count) << ", SubDevice " << m_sub_device
    << ", Cmd Class " << CommandClass() << ", Param ID " << m_param_id
    << ", Data Len " << m_data_length;
  str << ", Data ";
  for (unsigned int i = 0 ; i < m_data_length; i++)
    str << std::hex << std::setw(2) << static_cast<int>(m_data[i]) << " ";
  return str.str();
}


/*
 * Get the size of the raw data required to pack this RDM command
 */
unsigned int RDMCommand::Size() const {
  return sizeof(rdm_command_message) + m_data_length + CHECKSUM_LENGTH;
}


/*
 * Pack this command into an RDM message structure.
 * The packed data structure does not include the RDM start code (0xCC) because
 * sometimes devices / protocols keep this separate.
 */
bool RDMCommand::Pack(uint8_t *buffer, unsigned int *size) const {
  return Pack(buffer, size, m_source, m_transaction_number, m_port_id);
}


/*
 * Pack this command into an string.
 * The packed data does not include the RDM start code (0xCC) because
 * sometimes devices / protocols keep this separate.
 */
bool RDMCommand::Pack(string *data) const {
  return Pack(data, m_source, m_transaction_number, m_port_id);
}


/*
 * Pack this command into an RDM message structure with additional fields
 * The packed data structure does not include the RDM start code (0xCC) because
 * sometimes devices / protocols keep this separate.
 */
bool RDMCommand::Pack(uint8_t *buffer, unsigned int *size, const UID &source,
                      uint8_t transaction_number,
                      uint8_t port_id) const {
  if (*size < Size())
    return false;

  unsigned int packet_length = (sizeof(rdm_command_message) +
    m_data_length);  // size of packet excluding start code + checksum
  rdm_command_message message;
  message.sub_start_code = SUB_START_CODE;
  message.message_length = packet_length + 1;  // add in start code as well
  m_destination.Pack(message.destination_uid, UID::UID_SIZE);
  source.Pack(message.source_uid, UID::UID_SIZE);
  message.transaction_number = transaction_number;
  message.port_id = port_id;
  message.message_count = m_message_count;
  message.sub_device[0] = m_sub_device >> 8;
  message.sub_device[1] = m_sub_device & 0xff;
  message.command_class = CommandClass();
  message.param_id[0] = m_param_id >> 8;
  message.param_id[1] = m_param_id & 0xff;
  message.param_data_length = m_data_length;
  memcpy(buffer, &message, sizeof(message));
  memcpy(buffer + sizeof(rdm_command_message), m_data, m_data_length);

  uint16_t checksum = CalculateChecksum(buffer, packet_length);
  buffer[packet_length] = checksum >> 8;
  buffer[packet_length+1] = checksum & 0xff;

  *size = packet_length + CHECKSUM_LENGTH;
  return true;
}


bool RDMCommand::Pack(string *buffer,
                      const UID &source,
                      uint8_t transaction_number,
                      uint8_t port_id) const {
  if (!buffer)
    return false;

  uint8_t data[
    sizeof(rdm_command_message) + MAX_PARAM_DATA_LENGTH + CHECKSUM_LENGTH];
  unsigned int size = sizeof(data);
  bool r = Pack(data, &size, source, transaction_number, port_id);

  if (r)
    buffer->assign(reinterpret_cast<char*>(data), size);
  return r;
}


/*
 * Convert a block of RDM data to an RDMCommand object.
 * The data must not include the RDM start code.
 * @param data the raw RDM data, starting from the sub-start-code
 * @param length the length of the data
 */
const RDMCommand::rdm_command_message* RDMCommand::VerifyData(
    const uint8_t *data,
    unsigned int length) {
  if (!data) {
    OLA_WARN << "RDM data was null";
    return NULL;
  }

  if (length < sizeof(rdm_command_message)) {
    OLA_WARN << "RDM message is too small, needs to be at least " <<
      sizeof(rdm_command_message) << ", was " << length;
    return NULL;
  }

  const rdm_command_message *command_message = (
      reinterpret_cast<const rdm_command_message*>(data));

  unsigned int message_length = command_message->message_length;

  if (length < message_length + 1) {
    OLA_WARN << "RDM message is too small, needs to be " <<
      message_length + 1 << ", was " << length;
    return NULL;
  }

  uint16_t checksum = CalculateChecksum(data, message_length - 1);
  uint16_t actual_checksum = (data[message_length - 1] << 8) +
    data[message_length];

  if (actual_checksum != checksum) {
    OLA_WARN << "RDM checksum mismatch, was " << actual_checksum <<
      " but was supposed to be " << checksum;
    return NULL;
  }

  // check param length is valid here
  unsigned int block_size = length - sizeof(rdm_command_message) - 2;
  if (command_message->param_data_length > block_size) {
    OLA_WARN << "Param length " <<
      static_cast<int>(command_message->param_data_length) <<
      " exceeds remaining RDM message size of " << block_size;
    return NULL;
  }
  return command_message;
}


/*
 * Calculate the checksum of this packet
 */
uint16_t RDMCommand::CalculateChecksum(const uint8_t *data,
                                       unsigned int packet_length) {
  unsigned int checksum_value = START_CODE;
  for (unsigned int i = 0; i < packet_length; i++)
    checksum_value += data[i];
  return static_cast<uint16_t>(checksum_value);
}


/*
 * Convert the Command Class int to an enum
 */
RDMCommand::RDMCommandClass RDMCommand::ConvertCommandClass(
    uint8_t command_class) {
  switch (command_class) {
    case DISCOVER_COMMAND:
      return DISCOVER_COMMAND;
    case DISCOVER_COMMAND_RESPONSE:
      return DISCOVER_COMMAND_RESPONSE;
    case GET_COMMAND:
      return GET_COMMAND;
    case GET_COMMAND_RESPONSE:
      return GET_COMMAND_RESPONSE;
    case SET_COMMAND:
      return SET_COMMAND;
    case SET_COMMAND_RESPONSE:
      return SET_COMMAND_RESPONSE;
    default:
      return INVALID_COMMAND;
  }
}


/*
 * Inflate a request from some data
 */
RDMRequest* RDMRequest::InflateFromData(const uint8_t *data,
                                        unsigned int length) {
  const rdm_command_message *command_message = VerifyData(data, length);
  if (!command_message)
    return NULL;

  uint16_t sub_device = ((command_message->sub_device[0] << 8) +
    command_message->sub_device[1]);
  uint16_t param_id = ((command_message->param_id[0] << 8) +
    command_message->param_id[1]);

  RDMCommandClass command_class = ConvertCommandClass(
    command_message->command_class);

  switch (command_class) {
    case GET_COMMAND:
      return new RDMGetRequest(
          UID(command_message->source_uid),
          UID(command_message->destination_uid),
          command_message->transaction_number,  // transaction #
          command_message->port_id,  // port id
          command_message->message_count,  // message count
          sub_device,
          param_id,
          data + sizeof(rdm_command_message),
          command_message->param_data_length);  // data length
    case SET_COMMAND:
      return new RDMSetRequest(
          UID(command_message->source_uid),
          UID(command_message->destination_uid),
          command_message->transaction_number,  // transaction #
          command_message->port_id,  // port id
          command_message->message_count,  // message count
          sub_device,
          param_id,
          data + sizeof(rdm_command_message),
          command_message->param_data_length);  // data length
    default:
      OLA_WARN << "Expected a RDM request command but got " << command_class;
      return NULL;
  }
}


/**
 * Inflate from some data
 */
RDMRequest* RDMRequest::InflateFromData(const string &data) {
  return InflateFromData(reinterpret_cast<const uint8_t*>(data.data()),
                         data.size());
}


/**
 * Inflate a request from some data
 */
RDMResponse* RDMResponse::InflateFromData(const uint8_t *data,
                                          unsigned int length) {
  const rdm_command_message *command_message = VerifyData(data, length);
  if (!command_message)
    return NULL;

  uint16_t sub_device = ((command_message->sub_device[0] << 8) +
    command_message->sub_device[1]);
  uint16_t param_id = ((command_message->param_id[0] << 8) +
    command_message->param_id[1]);

  RDMCommandClass command_class = ConvertCommandClass(
    command_message->command_class);

  switch (command_class) {
    case GET_COMMAND_RESPONSE:
      return new RDMGetResponse(
          UID(command_message->source_uid),
          UID(command_message->destination_uid),
          command_message->transaction_number,  // transaction #
          command_message->port_id,  // port id
          command_message->message_count,  // message count
          sub_device,
          param_id,
          data + sizeof(rdm_command_message),
          command_message->param_data_length);  // data length
    case SET_COMMAND_RESPONSE:
      return new RDMSetResponse(
          UID(command_message->source_uid),
          UID(command_message->destination_uid),
          command_message->transaction_number,  // transaction #
          command_message->port_id,  // port id
          command_message->message_count,  // message count
          sub_device,
          param_id,
          data + sizeof(rdm_command_message),
          command_message->param_data_length);  // data length
    default:
      OLA_WARN << "Expected a RDM response command but got " << command_class;
      return NULL;
  }
}


/**
 * Inflate from some data
 */
RDMResponse* RDMResponse::InflateFromData(const string &data) {
  return InflateFromData(reinterpret_cast<const uint8_t*>(data.data()),
                         data.size());
}


/**
 * This combines two RDMResponses into one. It's used to combine the data from
 * two responses in an ACK_OVERFLOW session together.
 * @param response1 the first response.
 * @param response1 the second response.
 * @return A new response with the data from the first and second combined or
 * NULL if the size limit is reached.
 */
RDMResponse* RDMResponse::CombineResponses(const RDMResponse *response1,
                                           const RDMResponse *response2) {
  unsigned int combined_length = response1->ParamDataSize() +
    response2->ParamDataSize();
  // do some sort of checking
  if (combined_length > MAX_OVERFLOW_SIZE) {
    OLA_WARN << "ACK_OVERFLOW buffer size hit! Limit is " << MAX_OVERFLOW_SIZE
      << ", request size is " << combined_length;
    return NULL;
  } else if (response1->SourceUID() != response2->SourceUID()) {
    OLA_WARN << "Source UIDs don't match";
    return NULL;
  }

  uint8_t *combined_data = new uint8_t[combined_length];
  memcpy(combined_data, response1->ParamData(), response1->ParamDataSize());
  memcpy(combined_data + response1->ParamDataSize(),
         response2->ParamData(),
         response2->ParamDataSize());

  RDMResponse *response = NULL;
  if (response1->CommandClass() == GET_COMMAND_RESPONSE &&
      response2->CommandClass() == GET_COMMAND_RESPONSE) {
    response = new RDMGetResponse(
        response1->SourceUID(),
        response1->DestinationUID(),
        response1->TransactionNumber(),
        ola::rdm::ACK,
        response2->MessageCount(),
        response1->SubDevice(),
        response1->ParamId(),
        combined_data,
        combined_length);
  } else if (response1->CommandClass() == SET_COMMAND_RESPONSE &&
             response2->CommandClass() == SET_COMMAND_RESPONSE) {
    response = new RDMSetResponse(
        response1->SourceUID(),
        response1->DestinationUID(),
        response1->TransactionNumber(),
        ola::rdm::ACK,
        response2->MessageCount(),
        response1->SubDevice(),
        response1->ParamId(),
        combined_data,
        combined_length);
  } else {
    OLA_WARN << "Expected a RDM request command but got " <<
      std::hex << response1->CommandClass();
  }
  delete[] combined_data;
  return response;
}

// Helper functions follow
/*
 * Generate a NACK response with a reason code
 */
RDMResponse *NackWithReason(const RDMRequest *request,
                            rdm_nack_reason reason_enum) {
  uint16_t reason = ola::network::HostToNetwork(static_cast<uint16_t>(
    reason_enum));
  if (request->CommandClass() == ola::rdm::RDMCommand::GET_COMMAND) {
    return new ola::rdm::RDMGetResponse(
      request->DestinationUID(),
      request->SourceUID(),
      request->TransactionNumber(),
      NACK_REASON,
      0,
      request->SubDevice(),
      request->ParamId(),
      reinterpret_cast<uint8_t*>(&reason),
      sizeof(reason));
  } else  {
    return new ola::rdm::RDMSetResponse(
      request->DestinationUID(),
      request->SourceUID(),
      request->TransactionNumber(),
      NACK_REASON,
      0,
      request->SubDevice(),
      request->ParamId(),
      reinterpret_cast<uint8_t*>(&reason),
      sizeof(reason));
  }
}

/*
 * Generate a ACK Response with some data
 */
RDMResponse *GetResponseFromData(const RDMRequest *request,
                                 const uint8_t *data,
                                 unsigned int length,
                                 rdm_response_type type,
                                 uint8_t outstanding_messages) {
  // we can reuse GetResponseWithPid
  return GetResponseWithPid(request,
                            request->ParamId(),
                            data,
                            length,
                            type,
                            outstanding_messages);
}


/**
 * Generate a queued message response
 */
RDMResponse *GetResponseWithPid(const RDMRequest *request,
                                uint16_t pid,
                                const uint8_t *data,
                                unsigned int length,
                                rdm_response_type type,
                                uint8_t outstanding_messages) {
  if (request->CommandClass() == ola::rdm::RDMCommand::GET_COMMAND) {
    return new RDMGetResponse(
      request->DestinationUID(),
      request->SourceUID(),
      request->TransactionNumber(),
      type,
      outstanding_messages,
      request->SubDevice(),
      pid,
      data,
      length);
  } else {
    return new RDMSetResponse(
      request->DestinationUID(),
      request->SourceUID(),
      request->TransactionNumber(),
      type,
      outstanding_messages,
      request->SubDevice(),
      pid,
      data,
      length);
  }
}
}  // rdm
}  //  ola
