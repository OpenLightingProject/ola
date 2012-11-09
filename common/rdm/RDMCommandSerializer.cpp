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
 * RDMCommandSerializer.h
 * Write RDMCommands to a memory buffer.
 * Copyright (C) 2012 Simon Newton
 */

#include <stdint.h>
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
 * @param command the RDMCommand to pack
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


bool RDMCommandSerializer::PackWithParams(const RDMCommand &command,
                                          uint8_t *buffer,
                                          unsigned int *size,
                                          const UID &source,
                                          uint8_t transaction_number,
                                          uint8_t port_id) {
  unsigned int packet_length = RequiredSize(command);
  if (packet_length == 0 || *size < packet_length)
    return false;

  // ignore the checksum length for now
  packet_length -= CHECKSUM_LENGTH;

  RDMCommandHeader header;
  header.sub_start_code = SUB_START_CODE;
  header.message_length = packet_length + 1;  // add in start code as well
  command.DestinationUID().Pack(header.destination_uid, UID::UID_SIZE);
  source.Pack(header.source_uid, UID::UID_SIZE);
  header.transaction_number = transaction_number;
  header.port_id = port_id;
  header.message_count = command.MessageCount();
  header.sub_device[0] = command.SubDevice() >> 8;
  header.sub_device[1] = command.SubDevice() & 0xff;
  header.command_class = command.CommandClass();
  header.param_id[0] = command.ParamId() >> 8;
  header.param_id[1] = command.ParamId() & 0xff;
  header.param_data_length = command.ParamDataSize();
  memcpy(buffer, &header, sizeof(header));
  memcpy(buffer + sizeof(RDMCommandHeader), command.ParamData(),
         command.ParamDataSize());

  uint16_t checksum = START_CODE;
  for (unsigned int i = 0; i < packet_length; i++)
    checksum += buffer[i];
  buffer[packet_length] = checksum >> 8;
  buffer[packet_length + 1] = checksum & 0xff;

  *size = packet_length + CHECKSUM_LENGTH;
  return true;
}
}  // rdm
}  // ola
