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
#include "ola/strings/Format.h"
#include "ola/util/Utils.h"

namespace ola {
namespace rdm {

using std::string;
using ola::strings::ToHex;
using ola::utils::JoinUInt8;
using ola::utils::SplitUInt16;

// Internal Helper Functions
namespace {

/**
 * @brief Guess the CommandClass of an RDM message.
 * @param data a pointer to the RDM message (excluding the start code)
 * @param length length of the RDM data
 * @returns A RDMCommandClass value, which is set to INVALID_COMMAND if we
 * couldn't determine the message type.
 *
 * This doesn't perform any data checking (that's left to the Inflate* methods).
 */
RDMCommand::RDMCommandClass GuessMessageType(const uint8_t *data,
                                             unsigned int length) {
  static const unsigned int COMMAND_CLASS_OFFSET = 19;
  if (!data || length < COMMAND_CLASS_OFFSET + 1) {
    return RDMCommand::INVALID_COMMAND;
  }

  switch (data[COMMAND_CLASS_OFFSET]) {
    case RDMCommand::GET_COMMAND:
    case RDMCommand::GET_COMMAND_RESPONSE:
    case RDMCommand::SET_COMMAND:
    case RDMCommand::SET_COMMAND_RESPONSE:
    case RDMCommand::DISCOVER_COMMAND:
    case RDMCommand::DISCOVER_COMMAND_RESPONSE:
      return static_cast<RDMCommand::RDMCommandClass>(
          data[COMMAND_CLASS_OFFSET]);
    default:
      return RDMCommand::INVALID_COMMAND;
  }
}
}  // namespace

/**
 * @addtogroup rdm_command
 * @{
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
  if (m_data) {
    delete[] m_data;
  }
}

string RDMCommand::ToString() const {
  std::ostringstream str;
  str << m_source << " -> " << m_destination << ", Trans # " <<
    static_cast<int>(m_transaction_number) << ", Port ID " <<
    static_cast<int>(m_port_id) << ", Msg Cnt " <<
    static_cast<int>(m_message_count) << ", SubDevice " << m_sub_device
    << ", Cmd Class " << CommandClass() << ", Param ID " << m_param_id
    << ", Data Len " << m_data_length;
  str << ", Data ";
  for (unsigned int i = 0 ; i < m_data_length; i++) {
    str << std::hex << std::setw(2) << static_cast<int>(m_data[i]) << " ";
  }
  return str.str();
}

bool RDMCommand::operator==(const RDMCommand &other) const {
  if (SourceUID() == other.SourceUID() &&
      DestinationUID() == other.DestinationUID() &&
      TransactionNumber() == other.TransactionNumber() &&
      MessageCount() == other.MessageCount() &&
      SubDevice() == other.SubDevice() &&
      CommandClass() == other.CommandClass() &&
      ParamId() == other.ParamId() &&
      ParamDataSize() == other.ParamDataSize()) {
    return 0 == memcmp(ParamData(), other.ParamData(), ParamDataSize());
  }
  return false;
}

RDMCommand *RDMCommand::Inflate(const uint8_t *data, unsigned int length) {
  RDMCommandClass command_class = GuessMessageType(data, length);

  RDMStatusCode status_code = RDM_COMPLETED_OK;
  switch (command_class) {
    case RDMCommand::GET_COMMAND:
    case RDMCommand::SET_COMMAND:
      return RDMRequest::InflateFromData(data, length);
    case RDMCommand::GET_COMMAND_RESPONSE:
    case RDMCommand::SET_COMMAND_RESPONSE:
      return RDMResponse::InflateFromData(data, length, &status_code);
    case RDMCommand::DISCOVER_COMMAND:
      return RDMDiscoveryRequest::InflateFromData(data, length);
    case RDMCommand::DISCOVER_COMMAND_RESPONSE:
      return RDMDiscoveryResponse::InflateFromData(data, length);
    case RDMCommand::INVALID_COMMAND:
      return NULL;
  }
  return NULL;
}

uint8_t RDMCommand::MessageLength() const {
  // The size of packet including start code, excluding checksum
  return sizeof(RDMCommandHeader) + m_data_length + 1;
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
 * @return a RDMStatusCode
 */
RDMStatusCode RDMCommand::VerifyData(const uint8_t *data,
                                     size_t length,
                                     RDMCommandHeader *command_header) {
  if (length < sizeof(RDMCommandHeader)) {
    OLA_WARN << "RDM message is too small, needs to be at least " <<
      sizeof(RDMCommandHeader) << ", was " << length;
    return RDM_PACKET_TOO_SHORT;
  }

  if (!data) {
    OLA_WARN << "RDM data was null";
    return RDM_INVALID_RESPONSE;
  }

  memcpy(reinterpret_cast<uint8_t*>(command_header),
         data,
         sizeof(*command_header));

  if (command_header->sub_start_code != SUB_START_CODE) {
    OLA_WARN << "Sub start code mismatch, was 0x" << std::hex <<
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

RDMRequest::RDMRequest(const UID &source,
                       const UID &destination,
                       uint8_t transaction_number,
                       uint8_t port_id,
                       uint16_t sub_device,
                       RDMCommandClass command_class,
                       uint16_t param_id,
                       const uint8_t *data,
                       unsigned int length,
                       const OverrideOptions &options)
    : RDMCommand(source, destination, transaction_number, port_id,
                 options.message_count, sub_device, param_id, data, length),
      m_override_options(options),
      m_command_class(command_class) {
}

bool RDMRequest::IsDUB() const {
  return (CommandClass() == ola::rdm::RDMCommand::DISCOVER_COMMAND &&
          ParamId() == ola::rdm::PID_DISC_UNIQUE_BRANCH);
}

uint8_t RDMRequest::SubStartCode() const {
  return m_override_options.sub_start_code;
}

uint8_t RDMRequest::MessageLength() const {
  if (m_override_options.has_message_length) {
    return m_override_options.message_length;
  } else {
    return RDMCommand::MessageLength();
  }
}

uint16_t RDMRequest::Checksum(uint16_t checksum) const {
  return m_override_options.has_checksum ?
      m_override_options.checksum : checksum;
}

RDMRequest* RDMRequest::InflateFromData(const uint8_t *data,
                                        unsigned int length) {
  RDMCommandHeader command_message;
  RDMStatusCode status_code = VerifyData(data, length, &command_message);
  if (status_code != RDM_COMPLETED_OK) {
    return NULL;
  }

  uint16_t sub_device = JoinUInt8(command_message.sub_device[0],
                                  command_message.sub_device[1]);
  uint16_t param_id = JoinUInt8(command_message.param_id[0],
                                command_message.param_id[1]);
  RDMCommandClass command_class = ConvertCommandClass(
    command_message.command_class);

  OverrideOptions options;
  options.sub_start_code = command_message.sub_start_code;
  options.message_length = command_message.message_length;
  options.message_count = command_message.message_count;

  switch (command_class) {
    case DISCOVER_COMMAND:
      return new RDMDiscoveryRequest(
          UID(command_message.source_uid),
          UID(command_message.destination_uid),
          command_message.transaction_number,  // transaction #
          command_message.port_id,  // port id
          sub_device,
          param_id,
          data + sizeof(RDMCommandHeader),
          command_message.param_data_length,  // data length
          options);
    case GET_COMMAND:
      return new RDMGetRequest(
          UID(command_message.source_uid),
          UID(command_message.destination_uid),
          command_message.transaction_number,  // transaction #
          command_message.port_id,  // port id
          sub_device,
          param_id,
          data + sizeof(RDMCommandHeader),
          command_message.param_data_length,  // data length
          options);
    case SET_COMMAND:
      return new RDMSetRequest(
          UID(command_message.source_uid),
          UID(command_message.destination_uid),
          command_message.transaction_number,  // transaction #
          command_message.port_id,  // port id
          sub_device,
          param_id,
          data + sizeof(RDMCommandHeader),
          command_message.param_data_length,  // data length
          options);
    default:
      OLA_WARN << "Expected a RDM request command but got " << command_class;
      return NULL;
  }
}

RDMResponse* RDMResponse::InflateFromData(const uint8_t *data,
                                          size_t length,
                                          RDMStatusCode *status_code,
                                          const RDMRequest *request) {
  RDMCommandHeader command_message;
  *status_code = VerifyData(data, length, &command_message);
  if (*status_code != RDM_COMPLETED_OK) {
    return NULL;
  }

  UID source_uid(command_message.source_uid);
  UID destination_uid(command_message.destination_uid);
  uint16_t sub_device = ((command_message.sub_device[0] << 8) +
    command_message.sub_device[1]);
  RDMCommandClass command_class = ConvertCommandClass(
    command_message.command_class);

  if (request) {
    // check dest uid
    if (request->SourceUID() != destination_uid) {
      OLA_WARN << "The destination UID in the response doesn't match, got "
               << destination_uid << ", expected " << request->SourceUID();
      *status_code = RDM_DEST_UID_MISMATCH;
      return NULL;
    }

    // check src uid
    if (request->DestinationUID() != source_uid) {
      OLA_WARN << "The source UID in the response doesn't match, got "
               << source_uid << ", expected " << request->DestinationUID();
      *status_code = RDM_SRC_UID_MISMATCH;
      return NULL;
    }

    // check transaction #
    if (command_message.transaction_number != request->TransactionNumber()) {
      OLA_WARN << "Transaction numbers don't match, got "
               << static_cast<int>(command_message.transaction_number)
               << ", expected "
               << static_cast<int>(request->TransactionNumber());
      *status_code = RDM_TRANSACTION_MISMATCH;
      return NULL;
    }

    // check subdevice, but ignore if request was for all sub devices or
    // QUEUED_MESSAGE
    if (sub_device != request->SubDevice() &&
        request->SubDevice() != ALL_RDM_SUBDEVICES &&
         request->ParamId() != PID_QUEUED_MESSAGE) {
      OLA_WARN << "Sub device didn't match, got " << sub_device
               << ", expected " << request->SubDevice();
      *status_code = RDM_SUB_DEVICE_MISMATCH;
      return NULL;
    }

    // check command class
    if (request->CommandClass() == GET_COMMAND &&
        command_class != GET_COMMAND_RESPONSE &&
        request->ParamId() != PID_QUEUED_MESSAGE) {
      OLA_WARN << "Expected GET_COMMAND_RESPONSE, got "
               << ToHex(command_class);
      *status_code = RDM_COMMAND_CLASS_MISMATCH;
      return NULL;
    }

    if (request->CommandClass() == SET_COMMAND &&
        command_class != SET_COMMAND_RESPONSE) {
      OLA_WARN << "Expected SET_COMMAND_RESPONSE, got "
               << ToHex(command_class);
      *status_code = RDM_COMMAND_CLASS_MISMATCH;
      return NULL;
    }

    if (request->CommandClass() == DISCOVER_COMMAND &&
        command_class != DISCOVER_COMMAND_RESPONSE) {
      OLA_WARN << "Expected DISCOVER_COMMAND_RESPONSE, got "
               << ToHex(command_class);
      *status_code = RDM_COMMAND_CLASS_MISMATCH;
      return NULL;
    }
  }

  // check response type
  if (command_message.port_id > ACK_OVERFLOW) {
    OLA_WARN << "Response type isn't valid, got "
             << static_cast<int>(command_message.port_id);
    *status_code = RDM_INVALID_RESPONSE_TYPE;
    return NULL;
  }

  uint16_t param_id = JoinUInt8(command_message.param_id[0],
                                command_message.param_id[1]);
  uint8_t return_transaction_number = command_message.transaction_number;

  switch (command_class) {
    case DISCOVER_COMMAND_RESPONSE:
      *status_code = RDM_COMPLETED_OK;
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
      *status_code = RDM_COMPLETED_OK;
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
      *status_code = RDM_COMPLETED_OK;
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
      OLA_WARN << "Command class isn't valid, got " << ToHex(command_class);
      *status_code = RDM_INVALID_COMMAND_CLASS;
      return NULL;
  }
}

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


RDMResponse *NackWithReason(const RDMRequest *request,
                            rdm_nack_reason reason_enum,
                            uint8_t outstanding_messages) {
  uint16_t reason = ola::network::HostToNetwork(
      static_cast<uint16_t>(reason_enum));
  return GetResponseFromData(request,
                             reinterpret_cast<uint8_t*>(&reason),
                             sizeof(reason),
                             RDM_NACK_REASON,
                             outstanding_messages);
}

RDMResponse *GetResponseFromData(const RDMRequest *request,
                                 const uint8_t *data,
                                 unsigned int length,
                                 rdm_response_type type,
                                 uint8_t outstanding_messages) {
  // We can reuse GetResponseWithPid
  return GetResponseWithPid(request,
                            request->ParamId(),
                            data,
                            length,
                            type,
                            outstanding_messages);
}

RDMResponse *GetResponseWithPid(const RDMRequest *request,
                                uint16_t pid,
                                const uint8_t *data,
                                unsigned int length,
                                uint8_t type,
                                uint8_t outstanding_messages) {
  switch (request->CommandClass()) {
    case RDMCommand::GET_COMMAND:
      // coverity[SWAPPED_ARGUMENTS]
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
      // coverity[SWAPPED_ARGUMENTS]
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
      // coverity[SWAPPED_ARGUMENTS]
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
 * @brief Inflate a discovery request.
 */
RDMDiscoveryRequest* RDMDiscoveryRequest::InflateFromData(
    const uint8_t *data,
    unsigned int length) {
  RDMCommandHeader command_message;
  RDMStatusCode code = VerifyData(data, length, &command_message);
  if (code != RDM_COMPLETED_OK) {
    return NULL;
  }

  uint16_t sub_device = JoinUInt8(command_message.sub_device[0],
                                  command_message.sub_device[1]);
  uint16_t param_id = JoinUInt8(command_message.param_id[0],
                                command_message.param_id[1]);
  RDMCommandClass command_class = ConvertCommandClass(
    command_message.command_class);

  OverrideOptions options;
  options.sub_start_code = command_message.sub_start_code;
  options.message_length = command_message.message_length;
  options.message_count = command_message.message_count;

  if (command_class == DISCOVER_COMMAND) {
    return new RDMDiscoveryRequest(
        UID(command_message.source_uid),
        UID(command_message.destination_uid),
        command_message.transaction_number,  // transaction #
        command_message.port_id,  // port id
        sub_device,
        param_id,
        data + sizeof(RDMCommandHeader),
        command_message.param_data_length,  // data length
        options);
  } else {
    OLA_WARN << "Expected a RDM discovery request but got " << command_class;
    return NULL;
  }
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
                                 ROOT_RDM_DEVICE,
                                 PID_DISC_UNIQUE_BRANCH,
                                 param_data,
                                 length);
}


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
                                 ROOT_RDM_DEVICE,
                                 PID_DISC_MUTE,
                                 NULL,
                                 0);
}


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
                                   ROOT_RDM_DEVICE,
                                   PID_DISC_UN_MUTE,
                                   NULL,
                                   0);
}


/**
 * Inflate a discovery response.
 */
RDMDiscoveryResponse* RDMDiscoveryResponse::InflateFromData(
    const uint8_t *data,
    unsigned int length) {
  RDMCommandHeader command_message;
  RDMStatusCode code = VerifyData(data, length, &command_message);
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
    OLA_WARN << "Expected a RDM discovery response but got " << command_class;
    return NULL;
  }
}
/**@}*/
}  // namespace rdm
}  // namespace ola
