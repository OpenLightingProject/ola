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
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/UID.h>
#include <sstream>
#include <string>

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

    virtual ~RDMCommand();
    bool operator==(const RDMCommand &other) const;

    std::string ToString() const;

    friend ostream& operator<< (ostream &out, const RDMCommand &command) {
      return out << command.ToString();
    }

    // subclasses provide this
    virtual RDMCommandClass CommandClass() const = 0;

    // Accessors
    const UID& SourceUID() const { return m_source; }
    const UID& DestinationUID() const { return m_destination; }
    uint8_t TransactionNumber() const { return m_transaction_number; }
    uint8_t MessageCount() const { return m_message_count; }
    uint16_t SubDevice() const { return m_sub_device; }
    uint16_t ParamId() const { return m_param_id; }
    uint8_t *ParamData() const { return m_data; }
    unsigned int ParamDataSize() const { return m_data_length; }

    // Pack this RDMCommand into memory
    unsigned int Size() const;
    bool Pack(uint8_t *buffer, unsigned int *size) const;

    static const uint8_t START_CODE = 0xcc;
    enum { MAX_PARAM_DATA_LENGTH = 231 };

  protected:
    RDMCommand(const UID &source,
               const UID &destination,
               uint8_t transaction_number,
               uint8_t port_id,
               uint8_t message_count,
               uint16_t sub_device,
               uint16_t param_id,
               const uint8_t *data,
               unsigned int length);

    // don't use anything other than uint8_t here otherwise we can get
    // alignment issues.
    typedef struct {
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
    } rdm_command_message;

    static const rdm_command_message* VerifyData(const uint8_t *data,
                                                 unsigned int length);
    static RDMCommandClass ConvertCommandClass(uint8_t command_type);

    uint8_t m_port_id;

  private:
    UID m_source;
    UID m_destination;
    uint8_t m_transaction_number;
    uint8_t m_message_count;
    uint16_t m_sub_device;
    uint16_t m_param_id;
    uint8_t *m_data;
    unsigned int m_data_length;

    static const uint8_t SUB_START_CODE = 0x01;
    static const unsigned int CHECKSUM_LENGTH = 2;

    RDMCommand(const RDMCommand &other);
    bool operator=(const RDMCommand &other) const;
    RDMCommand& operator=(const RDMCommand &other);
    static uint16_t CalculateChecksum(const uint8_t *data,
                                      unsigned int packet_length);
};


/*
 * The subset of RDM Commands that represent requests
 */
class RDMRequest: public RDMCommand {
  public:
    RDMRequest(const UID &source,
               const UID &destination,
               uint8_t transaction_number,
               uint8_t port_id,
               uint8_t message_count,
               uint16_t sub_device,
               uint16_t param_id,
               const uint8_t *data,
               unsigned int length):
      RDMCommand(source,
                 destination,
                 transaction_number,
                 port_id,
                 message_count,
                 sub_device,
                 param_id,
                 data,
                 length) {
    }

    uint8_t PortId() const { return m_port_id; }

    virtual RDMRequest *Duplicate() const = 0;

    // Convert a block of data to an RDMCommand object
    static RDMRequest* InflateFromData(const uint8_t *data,
                                       unsigned int length);
};


template <RDMCommand::RDMCommandClass command_class>
class BaseRDMRequest: public RDMRequest {
  public:
    BaseRDMRequest(const UID &source,
                   const UID &destination,
                   uint8_t transaction_number,
                   uint8_t port_id,
                   uint8_t message_count,
                   uint16_t sub_device,
                   uint16_t param_id,
                   const uint8_t *data,
                   unsigned int length):
      RDMRequest(source,
                 destination,
                 transaction_number,
                 port_id,
                 message_count,
                 sub_device,
                 param_id,
                 data,
                 length) {
    }
    RDMCommandClass CommandClass() const { return command_class; }
    BaseRDMRequest<command_class> *Duplicate()
      const {
      return new BaseRDMRequest<command_class>(
        SourceUID(),
        DestinationUID(),
        TransactionNumber(),
        PortId(),
        MessageCount(),
        SubDevice(),
        ParamId(),
        ParamData(),
        ParamDataSize());
    }
};

typedef BaseRDMRequest<RDMCommand::GET_COMMAND> RDMGetRequest;
typedef BaseRDMRequest<RDMCommand::SET_COMMAND> RDMSetRequest;


/*
 * The subset of RDM Commands that represent requests
 */
class RDMResponse: public RDMCommand {
  public:
    RDMResponse(const UID &source,
                const UID &destination,
                uint8_t transaction_number,
                uint8_t response_type,
                uint8_t message_count,
                uint16_t sub_device,
                uint16_t param_id,
                const uint8_t *data,
                unsigned int length):
      RDMCommand(source,
                 destination,
                 transaction_number,
                 response_type,
                 message_count,
                 sub_device,
                 param_id,
                 data,
                 length) {
    }

    uint8_t ResponseType() const { return m_port_id; }

    // The maximum size of an ACK_OVERFLOW session that we'll buffer
    // 4k should be big enough for everyone ;)
    static const unsigned int MAX_OVERFLOW_SIZE = 4 << 10;

    // Convert a block of data to an RDMCommand object
    static RDMResponse* InflateFromData(const uint8_t *data,
                                        unsigned int length);

    // Combine two responses into one.
    static RDMResponse* CombineResponses(const RDMResponse *response1,
                                         const RDMResponse *response2);
};


template <RDMCommand::RDMCommandClass command_class>
class BaseRDMResponse: public RDMResponse {
  public:
    BaseRDMResponse(const UID &source,
                    const UID &destination,
                    uint8_t transaction_number,
                    uint8_t response_type,
                    uint8_t message_count,
                    uint16_t sub_device,
                    uint16_t param_id,
                    const uint8_t *data,
                    unsigned int length):
      RDMResponse(source,
                  destination,
                  transaction_number,
                  response_type,
                  message_count,
                  sub_device,
                  param_id,
                  data,
                  length) {
    }
    RDMCommandClass CommandClass() const { return command_class; }
};

typedef BaseRDMResponse<RDMCommand::GET_COMMAND_RESPONSE> RDMGetResponse;
typedef BaseRDMResponse<RDMCommand::SET_COMMAND_RESPONSE> RDMSetResponse;

// Helper functions for dealing with RDMCommands
RDMResponse *NackWithReason(const RDMRequest *request,
                            rdm_nack_reason reason);
RDMResponse *GetResponseWithData(const RDMRequest *request,
                                 const uint8_t *data,
                                 unsigned int length);
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_RDMCOMMAND_H_
