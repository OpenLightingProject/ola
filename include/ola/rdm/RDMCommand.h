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
 * RDMCommand.h
 * Representation of an RDM RDMCommand
 * Copyright (C) 2005-2010 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_RDMCOMMAND_H_
#define INCLUDE_OLA_RDM_RDMCOMMAND_H_

#include <stdint.h>
#include <ola/rdm/UID.h>

namespace ola {
namespace rdm {


/*
 * Represents a RDMCommand.
 */
class RDMCommand {
  public:
    typedef enum {
      DISCOVER_COMMAND = 0x10,
      DISCOVER_COMMAND_RESPONSE = 0x11,
      GET_COMMAND = 0x20,
      GET_COMMAND_RESPONSE = 0x21,
      SET_COMMAND = 0x30,
      SET_COMMAND_RESPONSE = 0x31,
      INVALID_COMMAND = 0xff,
    } RDMCommandClass;

    RDMCommand(const UID &source,
               const UID &destination,
               uint8_t transaction_number,
               uint8_t port_id,
               uint8_t message_count,
               uint16_t sub_device,
               RDMCommandClass command,
               uint16_t param_id,
               uint8_t *data,
               unsigned int length);

    RDMCommand(const RDMCommand &other);
    ~RDMCommand();

    bool operator==(const RDMCommand &other) const;

    // Accessors
    UID SourceUID() const { return m_source; }
    UID DestinationUID() const { return m_destination; }
    uint8_t TransactionNumber() const { return m_transaction_number; }
    uint8_t PortId() const { return m_port_id; }
    uint8_t MessageCount() const { return m_message_count; }
    uint16_t SubDevice() const { return m_sub_device; }
    RDMCommandClass CommandClass() const { return m_command; }
    uint16_t ParamId() const { return m_param_id; }
    uint8_t *ParamData() const { return m_data; }
    unsigned int ParamDataSize() const { return m_data_length; }

    // Pack this RDMCommand into memory
    unsigned int Size() const;
    bool Pack(uint8_t *buffer, unsigned int *size) const;

    // Convert a block of data to an RDMCommand object
    static RDMCommand *InflateFromData(const uint8_t *data,
                                       unsigned int length);

    static const uint8_t START_CODE = 0xcc;

  private:
    UID m_source;
    UID m_destination;
    uint8_t m_transaction_number;
    uint8_t m_port_id;
    uint8_t m_message_count;
    uint16_t m_sub_device;
    RDMCommandClass m_command;
    uint16_t m_param_id;
    uint8_t *m_data;
    unsigned int m_data_length;

    static const uint8_t SUB_START_CODE = 0x01;
    static const unsigned int CHECKSUM_LENGTH = 2;
    static const unsigned int MAX_PARAM_DATA_LENGTH = 231;

    // don't use anything other than uint8_t here otherwise we can get
    // alignment issues.
    struct rdm_command_message {
      uint8_t sub_start_code;
      uint8_t message_length;
      uint8_t destination_uid[UID::UID_SIZE];
      uint8_t source_uid[UID::UID_SIZE];
      uint8_t transaction_number;
      uint8_t port_id;
      uint8_t message_count;
      uint8_t sub_device[2];
      uint8_t command_class;
      uint8_t param_id[2];
      uint8_t param_data_length;
    };

    RDMCommand& operator=(const RDMCommand &other);
    static uint16_t CalculateChecksum(const uint8_t *data,
                                      unsigned int packet_length);
    static RDMCommandClass ConvertCommandClass(uint8_t command_type);
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_RDMCOMMAND_H_
