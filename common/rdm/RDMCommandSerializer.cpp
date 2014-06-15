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
 * RDMCommandSerializer.h
 * Write RDMCommands to a memory buffer.
 * Copyright (C) 2012 Simon Newton
 */

#include <stdint.h>
#include <string.h>
#include "ola/io/BigEndianStream.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/RDMPacket.h"

namespace ola {
namespace rdm {

/*
 * Returns the number of bytes required for the message.
 * @param command the RDMCommand
 * @returns the number of bytes required to pack this command, or 0 if this
 *   command contains more than 231 bytes of parameter data.
 */
unsigned int RDMCommandSerializer::RequiredSize(
    const RDMCommand &command) {
  if (command.ParamDataSize() > MAX_PARAM_DATA_LENGTH)
    return 0;
  return sizeof(RDMCommandHeader) + command.ParamDataSize() + CHECKSUM_LENGTH;
}


/**
 * Pack this RDMCommand into a memory buffer.
 * @param command the RDMCommand to pack
 * @param buffer a pointer to the memory location to use
 * @param size the size of the memory location. Update with the number of bytes
 *   written.
 * @returns true if successful, false otherwise.
 */
bool RDMCommandSerializer::Pack(const RDMCommand &command,
                                uint8_t *buffer,
                                unsigned int *size) {
  return PackWithParams(command, buffer, size, command.SourceUID(),
                        command.TransactionNumber(),
                        command.PortIdResponseType());
}

/**
 * Pack this RDMRequest into a memory buffer, using the supplied parameters to
 *   override what's in the request.
 * @param request the RDMRequest to pack
 * @param buffer a pointer to the memory location to use
 * @param size the size of the memory location. Update with the number of bytes
 *   written.
 * @param source the source UID
 * @param transaction_number the RDM transaction number
 * @param port_id the RDM port id
 * @returns true if successful, false otherwise.
 */
bool RDMCommandSerializer::Pack(const RDMRequest &request,
                                uint8_t *buffer,
                                unsigned int *size,
                                const UID &source,
                                uint8_t transaction_number,
                                uint8_t port_id) {
  return PackWithParams(request, buffer, size, source, transaction_number,
                        port_id);
}


/*
 * Write a RDMCommand to an IOStack as a RDM message.
 * @param command the RDMCommand
 * @param stack the IOStack to write to.
 * @returns true if the write was successful, false if the RDM command needs to
 * be fragmented.
 */
bool RDMCommandSerializer::Write(const RDMCommand &command,
                                 ola::io::IOStack *stack) {
  return WriteToStack(command, stack, command.SourceUID(),
                      command.TransactionNumber(),
                      command.PortIdResponseType());
}


/*
 * Write a RDMRequest to an IOStack as a RDM message.
 * @param request the RDMRequest
 * @param stack the IOStack to write to.
 * @param source the source UID
 * @param transaction_number the RDM transaction number
 * @param port_id the RDM port id
 * @returns true if the write was successful, false if the RDM request needs to
 * be fragmented.
 */
bool RDMCommandSerializer::Write(const RDMRequest &request,
                                 ola::io::IOStack *stack,
                                 const UID &source,
                                 uint8_t transaction_number,
                                 uint8_t port_id) {
  return WriteToStack(request, stack, source, transaction_number, port_id);
}


bool RDMCommandSerializer::PackWithParams(const RDMCommand &command,
                                          uint8_t *buffer,
                                          unsigned int *size,
                                          const UID &source,
                                          uint8_t transaction_number,
                                          uint8_t port_id) {
  unsigned int packet_length = RequiredSize(command);
  if (packet_length == 0 || *size < packet_length)
    return false;

  // The buffer pointer may not be aligned, so we incur a copy here.
  RDMCommandHeader header;
  PopulateHeader(&header, command, packet_length, source, transaction_number,
                 port_id);

  memcpy(buffer, &header, sizeof(header));
  memcpy(buffer + sizeof(RDMCommandHeader), command.ParamData(),
         command.ParamDataSize());

  uint16_t checksum = START_CODE;
  for (unsigned int i = 0; i < packet_length - CHECKSUM_LENGTH; i++)
    checksum += buffer[i];
  buffer[packet_length - CHECKSUM_LENGTH] = checksum >> 8;
  buffer[packet_length - CHECKSUM_LENGTH + 1] = checksum & 0xff;

  *size = packet_length;
  return true;
}


/**
 * Write an RDMCommand to a IOStack, excluding the START_CODE.
 * @param command the RDMCommand to write
 * @param stack the IOStack to write to
 * @param source the source UID
 * @param transaction_number the RDM transaction number
 * @param port_id the RDM port id
 * @returns true if successful, false if the parameter data is too large to fit
 *   into a single RDM message.
 */
bool RDMCommandSerializer::WriteToStack(const RDMCommand &command,
                                        ola::io::IOStack *stack,
                                        const UID &source,
                                        uint8_t transaction_number,
                                        uint8_t port_id) {
  unsigned int packet_length = RequiredSize(command);
  if (packet_length == 0)
    return false;

  RDMCommandHeader header;
  PopulateHeader(&header, command, packet_length, source, transaction_number,
                 port_id);

  uint16_t checksum = START_CODE;
  const uint8_t *ptr = reinterpret_cast<uint8_t*>(&header);
  for (unsigned int i = 0; i < sizeof(header); i++)
    checksum += ptr[i];
  ptr = command.ParamData();
  for (unsigned int i = 0; i < command.ParamDataSize(); i++)
    checksum += ptr[i];

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
 * @param the length of the packet excluding the start code.
 * @param source the source UID.
 * @param transaction_number the RDM transaction number
 * @param port_id the RDM port id
 */
void RDMCommandSerializer::PopulateHeader(RDMCommandHeader *header,
                                          const RDMCommand &command,
                                          unsigned int packet_length,
                                          const UID &source,
                                          uint8_t transaction_number,
                                          uint8_t port_id) {
  // ignore the checksum length for now
  packet_length -= CHECKSUM_LENGTH;

  header->sub_start_code = SUB_START_CODE;
  header->message_length = packet_length + 1;  // add in start code as well
  command.DestinationUID().Pack(header->destination_uid, UID::UID_SIZE);
  source.Pack(header->source_uid, UID::UID_SIZE);
  header->transaction_number = transaction_number;
  header->port_id = port_id;
  header->message_count = command.MessageCount();
  header->sub_device[0] = command.SubDevice() >> 8;
  header->sub_device[1] = command.SubDevice() & 0xff;
  header->command_class = command.CommandClass();
  header->param_id[0] = command.ParamId() >> 8;
  header->param_id[1] = command.ParamId() & 0xff;
  header->param_data_length = command.ParamDataSize();
}
}  // namespace rdm
}  // namespace ola
