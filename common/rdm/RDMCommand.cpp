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
 * RDMCommand.cpp
 * The RDMCommand class
 * Copyright (C) 2010 Simon Newton
 */

/**
 * @addtogroup rdm_command
 * @{
 * @file RDMCommand.cpp
 * @}
 */

#include <string.h>
#include <string>
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/UID.h"

namespace ola {
namespace rdm {

/**
 * @addtogroup rdm_command
 * @{
 */

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
  SetParamData(data, length);
}


RDMCommand::~RDMCommand() {
  if (m_data)
    delete[] m_data;
}


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


void RDMCommand::Write(ola::io::OutputStream *stream) const {
  unsigned int packet_length = (sizeof(RDMCommandHeader) +
    m_data_length);  // size of packet excluding start code + checksum

  RDMCommandHeader message;
  message.sub_start_code = SUB_START_CODE;
  message.message_length = packet_length + 1;  // add in start code as well
  m_destination.Pack(message.destination_uid, UID::UID_SIZE);
  m_source.Pack(message.source_uid, UID::UID_SIZE);
  message.transaction_number = m_transaction_number;
  message.port_id = m_port_id;
  message.message_count = m_message_count;
  message.sub_device[0] = m_sub_device >> 8;
  message.sub_device[1] = m_sub_device & 0xff;
  message.command_class = CommandClass();
  message.param_id[0] = m_param_id >> 8;
  message.param_id[1] = m_param_id & 0xff;
  message.param_data_length = m_data_length;

  unsigned int checksum_value = START_CODE;

  // checksum & write out the header
  const uint8_t *ptr = reinterpret_cast<uint8_t*>(&message);
  for (unsigned int i = 0; i != sizeof(message); i++)
    checksum_value += ptr[i];

  stream->Write(reinterpret_cast<uint8_t*>(&message), sizeof(message));

  // checksum & write out the data
  for (unsigned int i = 0; i != m_data_length; i++)
    checksum_value += m_data[i];
  stream->Write(m_data, m_data_length);

  uint16_t checksum = static_cast<uint16_t>(checksum_value);
  *stream << ola::network::HostToNetwork(checksum);
}


/**
 * Attempt to inflate RDM data (excluding the start code) into an RDMCommand
 * object. This is really only useful for sniffer-style programs.
 * @returns NULL if the RDM command is invalid.
 */
RDMCommand *RDMCommand::Inflate(const uint8_t *data, unsigned int length) {
  if (length < 21) {
    return NULL;
  }

  rdm_message_type type;
  RDMCommandClass command_class;

  if (!GuessMessageType(&type, &command_class, data, length))
    return NULL;

  rdm_response_code response_code = RDM_COMPLETED_OK;
  switch (command_class) {
    case RDMCommand::GET_COMMAND:
    case RDMCommand::SET_COMMAND:
      return RDMRequest::InflateFromData(data, length);
    case RDMCommand::GET_COMMAND_RESPONSE:
    case RDMCommand::SET_COMMAND_RESPONSE:
      return RDMResponse::InflateFromData(data, length, &response_code);
    case RDMCommand::DISCOVER_COMMAND:
      return RDMDiscoveryRequest::InflateFromData(data, length);
    case RDMCommand::DISCOVER_COMMAND_RESPONSE:
      return RDMDiscoveryResponse::InflateFromData(data, length);
    default:
      return NULL;
  }
}


/**
 * Set the parameter data
 */
void RDMCommand::SetParamData(const uint8_t *data, unsigned int length) {
  m_data_length = length;
  if (m_data_length > 0 && data != NULL) {
    if (m_data)
      delete[] m_data;

    m_data = new uint8_t[m_data_length];
    memcpy(m_data, data, m_data_length);
  }
}


/*
 * Convert a block of RDM data to an RDMCommand object.
 * The data must not include the RDM start code.
 * @param data the raw RDM data, starting from the sub-start-code
 * @param length the length of the data
 * @param command_header the RDMCommandHeader struct to copy the data to
 * @return a rdm_response_code
 */
rdm_response_code RDMCommand::VerifyData(
    const uint8_t *data,
    unsigned int length,
    RDMCommandHeader *command_header) {
  if (!data) {
    OLA_WARN << "RDM data was null";
    return RDM_INVALID_RESPONSE;
  }

  if (length < sizeof(RDMCommandHeader)) {
    OLA_WARN << "RDM message is too small, needs to be at least " <<
      sizeof(RDMCommandHeader) << ", was " << length;
    return RDM_PACKET_TOO_SHORT;
  }

  memcpy(reinterpret_cast<uint8_t*>(command_header),
         data,
         sizeof(*command_header));

  if (command_header->sub_start_code != SUB_START_CODE) {
    OLA_WARN << "Sub start code mis match, was 0x" << std::hex <<
      static_cast<int>(command_header->sub_start_code) << ", required 0x"
      << static_cast<int>(SUB_START_CODE);
    return RDM_WRONG_SUB_START_CODE;
  }

  unsigned int message_length = command_header->message_length;
  if (length < message_length + 1) {
    OLA_WARN << "RDM message is too small, needs to be " <<
      message_length + 1 << ", was " << length;
    return RDM_PACKET_LENGTH_MISMATCH;
  }

  uint16_t checksum = CalculateChecksum(data, message_length - 1);
  uint16_t actual_checksum = (data[message_length - 1] << 8) +
    data[message_length];

  if (actual_checksum != checksum) {
    OLA_WARN << "RDM checksum mismatch, was " << actual_checksum <<
      " but was supposed to be " << checksum;
    return RDM_CHECKSUM_INCORRECT;
  }

  // check param length is valid here
  unsigned int block_size = length - sizeof(RDMCommandHeader) - 2;
  if (command_header->param_data_length > block_size) {
    OLA_WARN << "Param length " <<
      static_cast<int>(command_header->param_data_length) <<
      " exceeds remaining RDM message size of " << block_size;
    return RDM_PARAM_LENGTH_MISMATCH;
  }
  return RDM_COMPLETED_OK;
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
  RDMCommandHeader command_message;
  rdm_response_code code = VerifyData(data, length, &command_message);
  if (code != RDM_COMPLETED_OK)
    return NULL;

  uint16_t sub_device = ((command_message.sub_device[0] << 8) +
    command_message.sub_device[1]);
  uint16_t param_id = ((command_message.param_id[0] << 8) +
    command_message.param_id[1]);

  RDMCommandClass command_class = ConvertCommandClass(
    command_message.command_class);

  switch (command_class) {
    case DISCOVER_COMMAND:
      return new RDMDiscoveryRequest(
          UID(command_message.source_uid),
          UID(command_message.destination_uid),
          command_message.transaction_number,  // transaction #
          command_message.port_id,  // port id
          command_message.message_count,  // message count
          sub_device,
          param_id,
          data + sizeof(RDMCommandHeader),
          command_message.param_data_length);  // data length
    case GET_COMMAND:
      return new RDMGetRequest(
          UID(command_message.source_uid),
          UID(command_message.destination_uid),
          command_message.transaction_number,  // transaction #
          command_message.port_id,  // port id
          command_message.message_count,  // message count
          sub_device,
          param_id,
          data + sizeof(RDMCommandHeader),
          command_message.param_data_length);  // data length
    case SET_COMMAND:
      return new RDMSetRequest(
          UID(command_message.source_uid),
          UID(command_message.destination_uid),
          command_message.transaction_number,  // transaction #
          command_message.port_id,  // port id
          command_message.message_count,  // message count
          sub_device,
          param_id,
          data + sizeof(RDMCommandHeader),
          command_message.param_data_length);  // data length
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
 * @param data the request data
 * @param length the length of the request data
 * @param response_code a pointer to a rdm_response_code to set
 * @param request an optional RDMRequest object that this response is for
 * @returns a new RDMResponse object, or NULL is this response is invalid
 */
RDMResponse* RDMResponse::InflateFromData(const uint8_t *data,
                                          unsigned int length,
                                          rdm_response_code *response_code,
                                          const RDMRequest *request) {
  if (request)
    return InflateFromData(data, length, response_code, request,
                           request->TransactionNumber());
  else
    return InflateFromData(data, length, response_code, request, 0);
}


RDMResponse* RDMResponse::InflateFromData(const uint8_t *data,
                                          unsigned int length,
                                          rdm_response_code *response_code,
                                          const RDMRequest *request,
                                          uint8_t transaction_number) {
  RDMCommandHeader command_message;
  *response_code = VerifyData(data, length, &command_message);
  if (*response_code != RDM_COMPLETED_OK)
    return NULL;

  UID source_uid(command_message.source_uid);
  UID destination_uid(command_message.destination_uid);
  uint16_t sub_device = ((command_message.sub_device[0] << 8) +
    command_message.sub_device[1]);
  RDMCommandClass command_class = ConvertCommandClass(
    command_message.command_class);

  if (request) {
    // check dest uid
    if (request->SourceUID() != destination_uid) {
      OLA_WARN << "The destination UID in the response doesn't match, got " <<
        destination_uid << ", expected " << request->SourceUID();
      *response_code = RDM_DEST_UID_MISMATCH;
      return NULL;
    }

    // check src uid
    if (request->DestinationUID() != source_uid) {
      OLA_WARN << "The source UID in the response doesn't match, got " <<
        source_uid << ", expected " << request->DestinationUID();
      *response_code = RDM_SRC_UID_MISMATCH;
      return NULL;
    }

    // check transaction #
    if (command_message.transaction_number != transaction_number) {
      OLA_WARN << "Transaction numbers don't match, got " <<
        static_cast<int>(command_message.transaction_number) << ", expected "
        << static_cast<int>(transaction_number);
      *response_code = RDM_TRANSACTION_MISMATCH;
      return NULL;
    }

    // check subdevice, but ignore if request was for all sub devices or
    // QUEUED_MESSAGE
    if (sub_device != request->SubDevice() &&
        request->SubDevice() != ALL_RDM_SUBDEVICES &&
         request->ParamId() != PID_QUEUED_MESSAGE) {
      OLA_WARN << "Sub device didn't match, got " << sub_device <<
        ", expected " << request->SubDevice();
      *response_code = RDM_SUB_DEVICE_MISMATCH;
      return NULL;
    }

    // check command class
    if (request->CommandClass() == GET_COMMAND &&
        command_class != GET_COMMAND_RESPONSE &&
        request->ParamId() != PID_QUEUED_MESSAGE) {
      OLA_WARN << "Expected GET_COMMAND_RESPONSE, got 0x" << std::hex <<
        command_class;
      *response_code = RDM_COMMAND_CLASS_MISMATCH;
      return NULL;
    }

    if (request->CommandClass() == SET_COMMAND &&
        command_class != SET_COMMAND_RESPONSE) {
      OLA_WARN << "Expected SET_COMMAND_RESPONSE, got 0x" << std::hex <<
        command_class;
      *response_code = RDM_COMMAND_CLASS_MISMATCH;
      return NULL;
    }

    if (request->CommandClass() == DISCOVER_COMMAND &&
        command_class != DISCOVER_COMMAND_RESPONSE) {
      OLA_WARN << "Expected DISCOVER_COMMAND_RESPONSE, got 0x" << std::hex <<
        command_class;
      *response_code = RDM_COMMAND_CLASS_MISMATCH;
      return NULL;
    }
  }

  // check response type
  if (command_message.port_id > ACK_OVERFLOW) {
    OLA_WARN << "Response type isn't valid, got " << command_message.port_id;
    *response_code = RDM_INVALID_RESPONSE_TYPE;
    return NULL;
  }

  uint16_t param_id = ((command_message.param_id[0] << 8) +
    command_message.param_id[1]);
  uint8_t return_transaction_number = (request ? transaction_number :
    command_message.transaction_number);

  switch (command_class) {
    case DISCOVER_COMMAND_RESPONSE:
      *response_code = RDM_COMPLETED_OK;
      return new RDMDiscoveryResponse(
          source_uid,
          destination_uid,
          return_transaction_number,  // transaction #
          command_message.port_id,  // port id
          command_message.message_count,  // message count
          sub_device,
          param_id,
          data + sizeof(RDMCommandHeader),
          command_message.param_data_length);  // data length
    case GET_COMMAND_RESPONSE:
      *response_code = RDM_COMPLETED_OK;
      return new RDMGetResponse(
          source_uid,
          destination_uid,
          return_transaction_number,  // transaction #
          command_message.port_id,  // port id
          command_message.message_count,  // message count
          sub_device,
          param_id,
          data + sizeof(RDMCommandHeader),
          command_message.param_data_length);  // data length
    case SET_COMMAND_RESPONSE:
      *response_code = RDM_COMPLETED_OK;
      return new RDMSetResponse(
          source_uid,
          destination_uid,
          return_transaction_number,  // transaction #
          command_message.port_id,  // port id
          command_message.message_count,  // message count
          sub_device,
          param_id,
          data + sizeof(RDMCommandHeader),
          command_message.param_data_length);  // data length
    default:
      OLA_WARN << "Command class isn't valid, got 0x" << std::hex <<
        command_class;
      *response_code = RDM_INVALID_COMMAND_CLASS;
      return NULL;
  }
}


/**
 * Inflate from some data
 */
RDMResponse* RDMResponse::InflateFromData(const string &data,
                                          rdm_response_code *response_code,
                                          const RDMRequest *request) {
  return InflateFromData(reinterpret_cast<const uint8_t*>(data.data()),
                         data.size(),
                         response_code,
                         request);
}


/**
 * Inflate from some data
 */
RDMResponse* RDMResponse::InflateFromData(const string &data,
                                          rdm_response_code *response_code,
                                          const RDMRequest *request,
                                          uint8_t transaction_number) {
  return InflateFromData(reinterpret_cast<const uint8_t*>(data.data()),
                         data.size(),
                         response_code,
                         request,
                         transaction_number);
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
        RDM_ACK,
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
        RDM_ACK,
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

/**
 * Guess the type of an RDM message, so we know whether we should unpack it as
 * a request or response. This doesn't perform any data checking (that's left
 * to the Inflate* methods).
 * @param type a pointer to a rdm_message_type variable which is set to
 * RDM_REQUEST or RDM_RESPONSE.
 * @param data a pointer to the rdm message (excluding the start code)
 * @param length length of the rdm data
 * @returns true if we could determine the type, false otherwise
 */
bool GuessMessageType(rdm_message_type *type_arg,
                      RDMCommand::RDMCommandClass *command_class_arg,
                      const uint8_t *data,
                      unsigned int length) {
  static const unsigned int COMMAND_CLASS_OFFSET = 19;
  if (!data || length < COMMAND_CLASS_OFFSET + 1)
    return false;

  rdm_message_type type;
  RDMCommand::RDMCommandClass command_class;

  switch (data[COMMAND_CLASS_OFFSET]) {
    case RDMCommand::GET_COMMAND:
      type = RDM_REQUEST;
      command_class = RDMCommand::GET_COMMAND;
      break;
    case RDMCommand::GET_COMMAND_RESPONSE:
      type = RDM_RESPONSE;
      command_class = RDMCommand::GET_COMMAND_RESPONSE;
      break;
    case RDMCommand::SET_COMMAND:
      type = RDM_REQUEST;
      command_class = RDMCommand::SET_COMMAND;
      break;
    case RDMCommand::SET_COMMAND_RESPONSE:
      type = RDM_RESPONSE;
      command_class = RDMCommand::SET_COMMAND_RESPONSE;
      break;
    case RDMCommand::DISCOVER_COMMAND:
      type = RDM_REQUEST;
      command_class = RDMCommand::DISCOVER_COMMAND;
      break;
    case RDMCommand::DISCOVER_COMMAND_RESPONSE:
      type = RDM_RESPONSE;
      command_class = RDMCommand::DISCOVER_COMMAND_RESPONSE;
      break;
    default:
      command_class = RDMCommand::INVALID_COMMAND;
      break;
  }

  if (command_class != RDMCommand::INVALID_COMMAND) {
    if (type_arg)
      *type_arg = type;
    if (command_class_arg)
      *command_class_arg = command_class;
    return true;
  }
  return false;
}


/*
 * Generate a NACK response with a reason code
 */
RDMResponse *NackWithReason(const RDMRequest *request,
                            rdm_nack_reason reason_enum,
                            uint8_t outstanding_messages) {
  uint16_t reason = ola::network::HostToNetwork(static_cast<uint16_t>(
    reason_enum));
  if (request->CommandClass() == ola::rdm::RDMCommand::GET_COMMAND) {
    return new ola::rdm::RDMGetResponse(
      request->DestinationUID(),
      request->SourceUID(),
      request->TransactionNumber(),
      RDM_NACK_REASON,
      outstanding_messages,
      request->SubDevice(),
      request->ParamId(),
      reinterpret_cast<uint8_t*>(&reason),
      sizeof(reason));
  } else  {
    return new ola::rdm::RDMSetResponse(
      request->DestinationUID(),
      request->SourceUID(),
      request->TransactionNumber(),
      RDM_NACK_REASON,
      outstanding_messages,
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
 * Construct a RDM response from a RDMRequest object.
 */
RDMResponse *GetResponseWithPid(const RDMRequest *request,
                                uint16_t pid,
                                const uint8_t *data,
                                unsigned int length,
                                uint8_t type,
                                uint8_t outstanding_messages) {
  switch (request->CommandClass()) {
    case RDMCommand::GET_COMMAND:
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
    case RDMCommand::SET_COMMAND:
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
    case RDMCommand::DISCOVER_COMMAND:
      return new RDMDiscoveryResponse(
        request->DestinationUID(),
        request->SourceUID(),
        request->TransactionNumber(),
        type,
        outstanding_messages,
        request->SubDevice(),
        pid,
        data,
        length);
     default:
      return NULL;
  }
}


/**
 * Inflate a discovery request.
 */
RDMDiscoveryRequest* RDMDiscoveryRequest::InflateFromData(
    const uint8_t *data,
    unsigned int length) {
  RDMCommandHeader command_message;
  rdm_response_code code = VerifyData(data, length, &command_message);
  if (code != RDM_COMPLETED_OK)
    return NULL;

  uint16_t sub_device = ((command_message.sub_device[0] << 8) +
    command_message.sub_device[1]);
  uint16_t param_id = ((command_message.param_id[0] << 8) +
    command_message.param_id[1]);

  RDMCommandClass command_class = ConvertCommandClass(
    command_message.command_class);

  if (command_class == DISCOVER_COMMAND) {
    return new RDMDiscoveryRequest(
        UID(command_message.source_uid),
        UID(command_message.destination_uid),
        command_message.transaction_number,  // transaction #
        command_message.port_id,  // port id
        command_message.message_count,  // message count
        sub_device,
        param_id,
        data + sizeof(RDMCommandHeader),
        command_message.param_data_length);  // data length
  } else {
    OLA_WARN << "Expected a RDM discovery request but got " << command_class;
    return NULL;
  }
}


/*
 * Inflate a discovery request from some data.
 */
RDMDiscoveryRequest* RDMDiscoveryRequest::InflateFromData(const string &data) {
  return InflateFromData(reinterpret_cast<const uint8_t*>(data.data()),
                         data.size());
}


/*
 * Create a new DUB request object.
 */
RDMDiscoveryRequest *NewDiscoveryUniqueBranchRequest(
    const UID &source,
    const UID &lower,
    const UID &upper,
    uint8_t transaction_number,
    uint8_t port_id) {
  uint8_t param_data[UID::UID_SIZE * 2];
  unsigned int length = sizeof(param_data);
  lower.Pack(param_data, length);
  upper.Pack(param_data + UID::UID_SIZE, length - UID::UID_SIZE);
  return new RDMDiscoveryRequest(source,
                                 UID::AllDevices(),
                                 transaction_number,
                                 port_id,
                                 0,  // message count
                                 ROOT_RDM_DEVICE,
                                 PID_DISC_UNIQUE_BRANCH,
                                 param_data,
                                 length);
};


/*
 * Create a new Mute Request Object.
 */
RDMDiscoveryRequest *NewMuteRequest(const UID &source,
                                    const UID &destination,
                                    uint8_t transaction_number,
                                    uint8_t port_id) {
  return new RDMDiscoveryRequest(source,
                                 destination,
                                 transaction_number,
                                 port_id,
                                 0,  // message count
                                 ROOT_RDM_DEVICE,
                                 PID_DISC_MUTE,
                                 NULL,
                                 0);
};


/**
 * Create a new UnMute request object.
 */
RDMDiscoveryRequest *NewUnMuteRequest(const UID &source,
                                      const UID &destination,
                                      uint8_t transaction_number,
                                      uint8_t port_id) {
    return new RDMDiscoveryRequest(source,
                                   destination,
                                   transaction_number,
                                   port_id,
                                   0,  // message count
                                   ROOT_RDM_DEVICE,
                                   PID_DISC_UN_MUTE,
                                   NULL,
                                   0);
};


/**
 * Inflate a discovery response.
 */
RDMDiscoveryResponse* RDMDiscoveryResponse::InflateFromData(
    const uint8_t *data,
    unsigned int length) {
  RDMCommandHeader command_message;
  rdm_response_code code = VerifyData(data, length, &command_message);
  if (code != RDM_COMPLETED_OK)
    return NULL;

  uint16_t sub_device = ((command_message.sub_device[0] << 8) +
    command_message.sub_device[1]);
  uint16_t param_id = ((command_message.param_id[0] << 8) +
    command_message.param_id[1]);

  RDMCommandClass command_class = ConvertCommandClass(
    command_message.command_class);

  if (command_class == DISCOVER_COMMAND_RESPONSE) {
    return new RDMDiscoveryResponse(
        UID(command_message.source_uid),
        UID(command_message.destination_uid),
        command_message.transaction_number,  // transaction #
        command_message.port_id,  // port id
        command_message.message_count,  // message count
        sub_device,
        param_id,
        data + sizeof(RDMCommandHeader),
        command_message.param_data_length);  // data length
  } else {
    OLA_WARN << "Expected a RDM discovery request but got " << command_class;
    return NULL;
  }
}


/*
 * Inflate a discovery response from some data.
 */
RDMDiscoveryResponse* RDMDiscoveryResponse::InflateFromData(
    const string &data) {
  return InflateFromData(reinterpret_cast<const uint8_t*>(data.data()),
                         data.size());
}
/**@}*/
}  // namespace rdm
}  // namespace ola
