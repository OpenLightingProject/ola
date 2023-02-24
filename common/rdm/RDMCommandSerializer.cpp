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
 * RDMCommandSerializer.cpp
 * Write RDMCommands to a memory buffer.
 * Copyright (C) 2012 Simon Newton
 */

#include <stdint.h>
#include <string.h>
#include "ola/io/BigEndianStream.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/RDMPacket.h"
#include "ola/util/Utils.h"

namespace ola {
namespace rdm {

using ola::utils::SplitUInt16;

unsigned int RDMCommandSerializer::RequiredSize(
    const RDMCommand &command) {
  if (command.ParamDataSize() > MAX_PARAM_DATA_LENGTH) {
    return 0;
  }
  // Don't use command.MessageLength() here, since it may be overridden.
  return sizeof(RDMCommandHeader) + command.ParamDataSize() + CHECKSUM_LENGTH;
}

bool RDMCommandSerializer::Pack(const RDMCommand &command,
                                ola::io::ByteString *output) {
  const unsigned int packet_length = RequiredSize(command);
  if (packet_length == 0) {
    return false;
  }

  size_t front = output->size();

  RDMCommandHeader header;
  PopulateHeader(&header, command);

  output->append(reinterpret_cast<const uint8_t*>(&header), sizeof(header));
  output->append(command.ParamData(), command.ParamDataSize());

  uint16_t checksum = START_CODE;
  for (unsigned int i = front; i < output->size(); i++) {
    checksum += (*output)[i];
  }
  checksum = command.Checksum(checksum);
  output->push_back(checksum >> 8);
  output->push_back(checksum & 0xff);
  return true;
}

bool RDMCommandSerializer::PackWithStartCode(const RDMCommand &command,
                                             ola::io::ByteString *output) {
  output->push_back(START_CODE);
  return Pack(command, output);
}

bool RDMCommandSerializer::Pack(const RDMCommand &command,
                                uint8_t *buffer,
                                unsigned int *size) {
  const unsigned int packet_length = RequiredSize(command);
  if (packet_length == 0 || *size < packet_length) {
    return false;
  }

  // The buffer pointer may not be aligned, so we incur a copy here.
  RDMCommandHeader header;
  PopulateHeader(&header, command);

  memcpy(buffer, &header, sizeof(header));
  memcpy(buffer + sizeof(RDMCommandHeader), command.ParamData(),
         command.ParamDataSize());

  uint16_t checksum = START_CODE;
  for (unsigned int i = 0; i < packet_length - CHECKSUM_LENGTH; i++) {
    checksum += buffer[i];
  }
  checksum = command.Checksum(checksum);
  buffer[packet_length - CHECKSUM_LENGTH] = checksum >> 8;
  buffer[packet_length - CHECKSUM_LENGTH + 1] = checksum & 0xff;

  *size = packet_length;
  return true;
}

bool RDMCommandSerializer::Write(const RDMCommand &command,
                                 ola::io::IOStack *stack) {
  const unsigned int packet_length = RequiredSize(command);
  if (packet_length == 0) {
    return false;
  }

  RDMCommandHeader header;
  PopulateHeader(&header, command);

  uint16_t checksum = START_CODE;
  const uint8_t *ptr = reinterpret_cast<uint8_t*>(&header);
  for (unsigned int i = 0; i < sizeof(header); i++) {
    checksum += ptr[i];
  }
  ptr = command.ParamData();
  for (unsigned int i = 0; i < command.ParamDataSize(); i++) {
    checksum += ptr[i];
  }

  checksum = command.Checksum(checksum);
  // now perform the write in reverse order (since it's a stack).
  ola::io::BigEndianOutputStream output(stack);
  output << checksum;
  output.Write(command.ParamData(), command.ParamDataSize());
  output.Write(reinterpret_cast<uint8_t*>(&header), sizeof(header));
  return true;
}

/**
 * Populate the RDMCommandHeader struct.
 * @param header a pointer to the RDMCommandHeader to populate
 * @param command the RDMCommand to use
 */
void RDMCommandSerializer::PopulateHeader(RDMCommandHeader *header,
                                          const RDMCommand &command) {
  header->sub_start_code = command.SubStartCode();
  header->message_length = command.MessageLength();
  command.DestinationUID().Pack(header->destination_uid, UID::UID_SIZE);
  command.SourceUID().Pack(header->source_uid, UID::UID_SIZE);
  header->transaction_number = command.TransactionNumber();
  header->port_id = command.PortIdResponseType();
  header->message_count = command.MessageCount();
  SplitUInt16(command.SubDevice(), &header->sub_device[0],
              &header->sub_device[1]);
  header->command_class = command.CommandClass();
  SplitUInt16(command.ParamId(), &header->param_id[0],
              &header->param_id[1]);
  header->param_data_length = command.ParamDataSize();
}
}  // namespace rdm
}  // namespace ola
