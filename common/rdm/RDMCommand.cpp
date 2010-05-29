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

#include "ola/Logging.h"
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
                       RDMCommandClass command,
                       uint16_t param_id,
                       uint8_t *data,
                       unsigned int length):
    m_source(source),
    m_destination(destination),
    m_transaction_number(transaction_number),
    m_port_id(port_id),
    m_message_count(message_count),
    m_sub_device(sub_device),
    m_command(command),
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
 * Copy constructor
 */
RDMCommand::RDMCommand(const RDMCommand &other)
    : m_source(other.m_source),
      m_destination(other.m_destination),
      m_transaction_number(other.m_transaction_number),
      m_port_id(other.m_port_id),
      m_message_count(other.m_message_count),
      m_sub_device(other.m_sub_device),
      m_command(other.m_command),
      m_param_id(other.m_param_id),
      m_data(NULL),
      m_data_length(other.m_data_length) {
  if (m_data_length && other.m_data) {
    m_data = new uint8_t[other.m_data_length];
    memcpy(m_data, other.m_data, m_data_length);
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
 * Assignment operator
 */
RDMCommand& RDMCommand::operator=(const RDMCommand &other) {
  if (this != &other) {
    m_source = other.m_source;
    m_destination = other.m_destination;
    m_transaction_number = other.m_transaction_number;
    m_port_id = other.m_port_id;
    m_message_count = other.m_message_count;
    m_sub_device = other.m_sub_device;
    m_command = other.m_command;
    m_param_id = other.m_param_id;
    m_data_length = other.m_data_length;
    if (m_data_length && other.m_data) {
      m_data = new uint8_t[other.m_data_length];
      memcpy(m_data, other.m_data, m_data_length);
    } else {
      m_data = NULL;
    }
  }

  return *this;
}


/*
 * Comparison
 */
bool RDMCommand::operator==(const RDMCommand &other) const {
  return (
    m_source == other.m_source &&
    m_destination == other.m_destination &&
    m_transaction_number == other.m_transaction_number &&
    m_port_id == other.m_port_id &&
    m_message_count == other.m_message_count &&
    m_sub_device == other.m_sub_device &&
    m_command == other.m_command &&
    m_param_id == other.m_param_id &&
    m_data_length == other.m_data_length &&
    memcmp(m_data, other.m_data, m_data_length) == 0);
}


/*
 * Get the size of the raw data required to pack this RDM command
 */
unsigned int RDMCommand::Size() const {
  return sizeof(struct rdm_command_message) + m_data_length + CHECKSUM_LENGTH;
}


/*
 * Pack this command into an RDM message structure.
 * The packed data structure does not include the RDM start code (0xCC) because
 * sometimes devices / protocols keep this separate.
 */
bool RDMCommand::Pack(uint8_t *buffer, unsigned int *size) const {
  if (*size < Size())
    return false;

  unsigned int packet_length = (sizeof(struct rdm_command_message) +
    m_data_length);  // size of packet excluding start code + checksum
  struct rdm_command_message message;
  message.sub_start_code = SUB_START_CODE;
  message.message_length = packet_length + 1;  // add in start code as well
  m_destination.Pack(message.destination_uid, UID::UID_SIZE);
  m_source.Pack(message.source_uid, UID::UID_SIZE);
  message.transaction_number = m_transaction_number;
  message.port_id = m_port_id;
  message.message_count = m_message_count;
  message.sub_device[0] = m_sub_device >> 8;
  message.sub_device[1] = m_sub_device & 0xff;
  message.command_class = m_command;
  message.param_id[0] = m_param_id >> 8;
  message.param_id[1] = m_param_id & 0xff;
  message.param_data_length = m_data_length;
  memcpy(buffer, &message, sizeof(message));
  memcpy(buffer + sizeof(struct rdm_command_message), m_data, m_data_length);

  uint16_t checksum = CalculateChecksum(buffer, packet_length);
  buffer[packet_length] = checksum >> 8;
  buffer[packet_length+1] = checksum & 0xff;

  *size = packet_length + CHECKSUM_LENGTH;
  return true;
}


/*
 * Convert a block of RDM data to an RDMCommand object.
 * The data must not include the RDM start code.
 * @param data the raw RDM data, starting from the sub-start-code
 * @param length the length of the data
 */
RDMCommand *RDMCommand::InflateFromData(const uint8_t *data,
                                        unsigned int length) {
  if (!data) {
    OLA_WARN << "RDM data was null";
    return NULL;
  }

  if (length < sizeof(struct rdm_command_message)) {
    OLA_WARN << "RDM message is too small, needs to be at least " <<
      sizeof(struct rdm_command_message) << ", was " << length;
    return NULL;
  }

  const struct rdm_command_message *command_message = (
      reinterpret_cast<const struct rdm_command_message*>(data));

  uint8_t message_length = command_message->message_length;

  if (length < message_length + 1) {
    OLA_WARN << "RDM message is too small, needs to be " <<
      message_length + 1 << ", was " << length;
    return NULL;
  }

  uint16_t checksum = CalculateChecksum(data, message_length - 1);
  uint16_t actual_checksum = (data[message_length - 1] << 8) +
    data[message_length];

  if (actual_checksum != checksum) {
    OLA_WARN << "RDM checmsum mismatch, was " << actual_checksum <<
      " but was supposed to be " << checksum;
    return NULL;
  }

  RDMCommandClass command_class = ConvertCommandClass(
    command_message->command_class);
  if (command_class == INVALID_COMMAND) {
    OLA_WARN << "Got unknown RDM command class: " <<
        command_message->command_class;
    return NULL;
  }

  uint16_t sub_device = ((command_message->sub_device[0] << 8) +
    command_message->sub_device[1]);
  uint16_t param_id = ((command_message->param_id[0] << 8) +
    command_message->param_id[1]);

  // check param length is valid here
  unsigned int block_size = length - sizeof(struct rdm_command_message) - 2;
  if (command_message->param_data_length > block_size) {
    OLA_WARN << "Param length " <<
      static_cast<int>(command_message->param_data_length) <<
      " exceeds remaining RDM message size of " << block_size;
    return NULL;
  }

  return new RDMCommand(
      UID(command_message->source_uid),
      UID(command_message->destination_uid),
      command_message->transaction_number,  // transaction #
      command_message->port_id,  // port id
      command_message->message_count,  // message count
      sub_device,
      command_class,
      param_id,
      NULL,  // data
      command_message->param_data_length);  // data length
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
}  // rdm
}  //  ola
