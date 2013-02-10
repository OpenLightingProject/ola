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
 * RDMCommand.h
 * All the classes that represent RDM commands.
 * Copyright (C) 2005-2012 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_RDMCOMMAND_H_
#define INCLUDE_OLA_RDM_RDMCOMMAND_H_

#include <stdint.h>
#include <ola/io/OutputStream.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMPacket.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/rdm/UID.h>
#include <sstream>
#include <string>

namespace ola {
namespace rdm {


typedef enum {
  RDM_REQUEST,
  RDM_RESPONSE,
  RDM_INVALID,  // should never occur
} rdm_message_type;


/*
 * The base class that all RDM commands inherit from.
 * RDMCommands are immutable.
 * RDMCommands may hold more than 231 bytes of data. Use the
 * RDMCommandSerializer class if you want the wire format.
 * TODO: make these reference counted so that fan out during broadcasts isn't
 *   as expensive.
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

    /*
     * This doesn't correspond to a field in the RDM message, but it's a useful
     * shortcut to determine the direction of the message.
     * @return RDM_REQUEST or RDM_RESPONSE.
     */
    rdm_message_type CommandType() const {
      switch (CommandClass()) {
        case DISCOVER_COMMAND:
        case GET_COMMAND:
        case SET_COMMAND:
          return RDM_REQUEST;
        case DISCOVER_COMMAND_RESPONSE:
        case GET_COMMAND_RESPONSE:
        case SET_COMMAND_RESPONSE:
          return RDM_RESPONSE;
        default:
          return RDM_INVALID;
      }
    }

    // String methods.
    std::string ToString() const;

    friend ostream& operator<< (ostream &out, const RDMCommand &command) {
      return out << command.ToString();
    }

    // The CommandClass for the RDM message. Provided by the subclasses.
    virtual RDMCommandClass CommandClass() const = 0;

    // Accessors
    const UID& SourceUID() const { return m_source; }
    const UID& DestinationUID() const { return m_destination; }
    uint8_t TransactionNumber() const { return m_transaction_number; }
    uint8_t MessageCount() const { return m_message_count; }
    uint16_t SubDevice() const { return m_sub_device; }
    uint8_t PortIdResponseType() const { return m_port_id; }
    uint16_t ParamId() const { return m_param_id; }
    uint8_t *ParamData() const { return m_data; }
    unsigned int ParamDataSize() const { return m_data_length; }

    void Write(ola::io::OutputStream *stream) const;

    static const uint8_t START_CODE = 0xcc;

  protected:
    uint8_t m_port_id;

    RDMCommand(const UID &source,
               const UID &destination,
               uint8_t transaction_number,
               uint8_t port_id,
               uint8_t message_count,
               uint16_t sub_device,
               uint16_t param_id,
               const uint8_t *data,
               unsigned int length);

    void SetParamData(const uint8_t *data, unsigned int length);

    static rdm_response_code VerifyData(
        const uint8_t *data,
        unsigned int length,
        RDMCommandHeader *command_message);

    static RDMCommandClass ConvertCommandClass(uint8_t command_type);

  private:
    UID m_source;
    UID m_destination;
    uint8_t m_transaction_number;
    uint8_t m_message_count;
    uint16_t m_sub_device;
    uint16_t m_param_id;
    uint8_t *m_data;
    unsigned int m_data_length;

    RDMCommand(const RDMCommand &other);
    bool operator=(const RDMCommand &other) const;
    RDMCommand& operator=(const RDMCommand &other);
    static uint16_t CalculateChecksum(const uint8_t *data,
                                      unsigned int packet_length);
};


/*
 * RDM Commands that represent requests (GET, SET or DISCOVER).
 */
class RDMRequest: public RDMCommand {
  public:
    RDMRequest(const UID &source,
               const UID &destination,
               uint8_t transaction_number,
               uint8_t port_id,
               uint8_t message_count,
               uint16_t sub_device,
               RDMCommandClass command_class,
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
                 length),
      m_command_class(command_class) {
    }

    RDMCommandClass CommandClass() const { return m_command_class; }
    uint8_t PortId() const { return m_port_id; }

    virtual RDMRequest *Duplicate() const {
      return DuplicateWithControllerParams(
        SourceUID(),
        TransactionNumber(),
        PortId());
    }

    virtual RDMRequest *DuplicateWithControllerParams(
        const UID &source,
        uint8_t transaction_number,
        uint8_t port_id) const {
      return new RDMRequest(
        source,
        DestinationUID(),
        transaction_number,
        port_id,
        MessageCount(),
        SubDevice(),
        m_command_class,
        ParamId(),
        ParamData(),
        ParamDataSize());
    }

    // Convert a block of data to an RDMCommand object
    static RDMRequest* InflateFromData(const uint8_t *data,
                                       unsigned int length);
    static RDMRequest* InflateFromData(const string &data);

  private:
    RDMCommandClass m_command_class;
};


/**
 * The parent class for GET/SET requests.
 */
class RDMGetSetRequest: public RDMRequest {
  public:
    RDMGetSetRequest(const UID &source,
                     const UID &destination,
                     uint8_t transaction_number,
                     uint8_t port_id,
                     uint8_t message_count,
                     uint16_t sub_device,
                     RDMCommandClass command_class,
                     uint16_t param_id,
                     const uint8_t *data,
                     unsigned int length):
      RDMRequest(source,
                 destination,
                 transaction_number,
                 port_id,
                 message_count,
                 sub_device,
                 command_class,
                 param_id,
                 data,
                 length) {
    }
};


template <RDMCommand::RDMCommandClass command_class>
class BaseRDMRequest: public RDMGetSetRequest {
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
      RDMGetSetRequest(source,
                       destination,
                       transaction_number,
                       port_id,
                       message_count,
                       sub_device,
                       command_class,
                       param_id,
                       data,
                       length) {
    }
    RDMCommandClass CommandClass() const { return command_class; }
    BaseRDMRequest<command_class> *Duplicate()
      const {
      return DuplicateWithControllerParams(
        SourceUID(),
        TransactionNumber(),
        PortId());
    }

    BaseRDMRequest<command_class> *DuplicateWithControllerParams(
        const UID &source,
        uint8_t transaction_number,
        uint8_t port_id) const {
      return new BaseRDMRequest<command_class>(
        source,
        DestinationUID(),
        transaction_number,
        port_id,
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
 * The subset of RDM Commands that represent responses (GET, SET or DISCOVER).
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

    // Convert a block of data to an RDMResponse object
    static RDMResponse* InflateFromData(const uint8_t *data,
                                        unsigned int length,
                                        rdm_response_code *response_code,
                                        const RDMRequest *request = NULL);
    static RDMResponse* InflateFromData(const uint8_t *data,
                                        unsigned int length,
                                        rdm_response_code *response_code,
                                        const RDMRequest *request,
                                        uint8_t transaction_number);
    static RDMResponse* InflateFromData(const string &data,
                                        rdm_response_code *response_code,
                                        const RDMRequest *request = NULL);
    static RDMResponse* InflateFromData(const string &data,
                                        rdm_response_code *response_code,
                                        const RDMRequest *request,
                                        uint8_t transaction_number);

    // Combine two responses into one.
    static RDMResponse* CombineResponses(const RDMResponse *response1,
                                         const RDMResponse *response2);
};


/**
 * The base class for GET/SET responses
 */
class RDMGetSetResponse: public RDMResponse {
  public:
    RDMGetSetResponse(const UID &source,
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
};


template <RDMCommand::RDMCommandClass command_class>
class BaseRDMResponse: public RDMGetSetResponse {
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
      RDMGetSetResponse(source,
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
// These are mostly used with the RDM-TRI & dummy plugin
bool GuessMessageType(rdm_message_type *type,
                      RDMCommand::RDMCommandClass *command_class,
                      const uint8_t *data,
                      unsigned int length);
RDMResponse *NackWithReason(const RDMRequest *request,
                            rdm_nack_reason reason);
RDMResponse *GetResponseFromData(const RDMRequest *request,
                                 const uint8_t *data = NULL,
                                 unsigned int length = 0,
                                 rdm_response_type type = RDM_ACK,
                                 uint8_t outstanding_messages = 0);

RDMResponse *GetResponseWithPid(const RDMRequest *request,
                                uint16_t pid,
                                const uint8_t *data,
                                unsigned int length,
                                uint8_t type = RDM_ACK,
                                uint8_t outstanding_messages = 0);

/**
 * An RDM request of type DISCOVER_COMMAND
 */
class RDMDiscoveryRequest: public RDMRequest {
  public:
    RDMDiscoveryRequest(const UID &source,
                        const UID &destination,
                        uint8_t transaction_number,
                        uint8_t port_id,
                        uint8_t message_count,
                        uint16_t sub_device,
                        uint16_t param_id,
                        const uint8_t *data,
                        unsigned int length)
        : RDMRequest(source,
                     destination,
                     transaction_number,
                     port_id,
                     message_count,
                     sub_device,
                     DISCOVER_COMMAND,
                     param_id,
                     data,
                     length) {
    }

    uint8_t PortId() const { return m_port_id; }

    static RDMDiscoveryRequest* InflateFromData(const uint8_t *data,
                                                unsigned int length);
    static RDMDiscoveryRequest* InflateFromData(const string &data);
};


// Because the number of discovery requests is small (3 type) we provide a
// helper method for each here.
/*
 * Create a new DUB request object.
 */
RDMDiscoveryRequest *NewDiscoveryUniqueBranchRequest(
    const UID &source,
    const UID &lower,
    const UID &upper,
    uint8_t transaction_number,
    uint8_t port_id = 1);


/*
 * Create a new Mute Request Object.
 */
RDMDiscoveryRequest *NewMuteRequest(const UID &source,
                                    const UID &destination,
                                    uint8_t transaction_number,
                                    uint8_t port_id = 1);

/**
 * Create a new UnMute request object.
 */
RDMDiscoveryRequest *NewUnMuteRequest(const UID &source,
                                      const UID &destination,
                                      uint8_t transaction_number,
                                      uint8_t port_id = 1);


/**
 * An RDM response of type DISCOVER_COMMAND
 */
class RDMDiscoveryResponse: public RDMResponse {
  public:
    RDMDiscoveryResponse(const UID &source,
                         const UID &destination,
                         uint8_t transaction_number,
                         uint8_t port_id,
                         uint8_t message_count,
                         uint16_t sub_device,
                         uint16_t param_id,
                         const uint8_t *data,
                         unsigned int length)
        : RDMResponse(source,
                      destination,
                      transaction_number,
                      port_id,
                      message_count,
                      sub_device,
                      param_id,
                      data,
                      length) {
    }

    RDMCommandClass CommandClass() const { return DISCOVER_COMMAND_RESPONSE; }

    static RDMDiscoveryResponse* InflateFromData(const uint8_t *data,
                                                 unsigned int length);
    static RDMDiscoveryResponse* InflateFromData(const string &data);
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_RDMCOMMAND_H_
