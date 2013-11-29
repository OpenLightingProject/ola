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


/**
 * @addtogroup rdm_command
 * @{
 * @file RDMCommand.h
 * @brief Classes that represent RDM commands.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_RDMCOMMAND_H_
#define INCLUDE_OLA_RDM_RDMCOMMAND_H_

#include <stdint.h>
#include <ola/io/OutputStream.h>
#include <ola/rdm/CommandPrinter.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMPacket.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/rdm/UID.h>
#include <sstream>
#include <string>

namespace ola {
namespace rdm {

/**
 * @addtogroup rdm_command
 * @{
 */

/**
 * @brief An OLA specific enum that allows us to tell if a command is a request
 * or a response. See RDMCommandClass for more information.
 */
typedef enum {
  RDM_REQUEST, /**< Request */
  RDM_RESPONSE, /**< Response */
  RDM_INVALID,  /**< Invalid */
} rdm_message_type;


/**
 * @brief The base class that all RDM commands inherit from.
 * @note RDMCommands are immutable.
 * @note RDMCommands may hold more than 231 bytes of data. Use the
 * RDMCommandSerializer class if you want the wire format.
 */
/*
 *
 * TODO: make these reference counted so that fan out during broadcasts isn't
 *   as expensive.
 */
class RDMCommand {
 public:
    /**
     * @brief A set of values representing CommandClasses in E1.20.
     * @note Please see section 6.2.10 of ANSI E1.20 for more information.
     */
    typedef enum {
      DISCOVER_COMMAND = 0x10, /**< Discovery Command */
      DISCOVER_COMMAND_RESPONSE = 0x11, /**< Discovery Response */
      GET_COMMAND = 0x20, /**< Get Command */
      GET_COMMAND_RESPONSE = 0x21, /**< Get Response */
      SET_COMMAND = 0x30, /**< Set Command */
      SET_COMMAND_RESPONSE = 0x31, /**< Set Response */
      INVALID_COMMAND = 0xff, /**< Invalid Command, specific to OLA */
    } RDMCommandClass;

    virtual ~RDMCommand();

    /**
     * @brief Equality Operator
     * @param other is the other RDMCommand you wish to compare against.
     */
    bool operator==(const RDMCommand &other) const;

    /**
     * @brief Used as a quick way to determine if the command is a request or a
     * response.
     *
     * @note This doesn't correspond to a field in the RDM message, but it's a
     * useful shortcut to determine the direction of the message.
     * @returns RDM_REQUEST or RDM_RESPONSE.
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

    /**
     * @name String Methods
     * @{
     */

    /**
     * @brief Create a string from the RDMCommand object.
     * @returns A string containing the source and destination UIDS, transaction
     * number, port ID, Message count, Sub Device, Cmd Class, Param ID, Data,
     * and a raw string of the parameter data.
     * @param out ostream to output to
     * @param command is the RDMCommand to print
     */
    std::string ToString() const;

    friend ostream& operator<< (ostream &out, const RDMCommand &command) {
      return out << command.ToString();
    }

    /** @} */

    /**
     * @brief a virtual method to return the current CommmandClass.
     */
    virtual RDMCommandClass CommandClass() const = 0;

    /**
     * @name Accessors
     * @{
     */

    /** @brief Returns the Source UID of the RDMCommand */
    const UID& SourceUID() const { return m_source; }

    /** @brief Returns the Destination UID of the RDMCommand */
    const UID& DestinationUID() const { return m_destination; }

    /** @brief Returns the Transaction Number of the RDMCommand */
    uint8_t TransactionNumber() const { return m_transaction_number; }

    /** @brief Returns the Message Count of the RDMCommand */
    uint8_t MessageCount() const { return m_message_count; }

    /** @brief Returns the SubDevice of the RDMCommand */
    uint16_t SubDevice() const { return m_sub_device; }

    /** @brief Returns the Port ID of the RDMCommand */
    uint8_t PortIdResponseType() const { return m_port_id; }

    /** @brief Returns the Parameter ID of the RDMCommand */
    uint16_t ParamId() const { return m_param_id; }

    /** @brief Returns the Parameter Data of the RDMCommand */
    uint8_t *ParamData() const { return m_data; }

    /** @brief Returns the Size of the Parameter Data of the RDMCommand */
    unsigned int ParamDataSize() const { return m_data_length; }

    /** @} */

    /**
     * @brief Used to print the data in an RDM Command to a CommandPrinter
     * @param print CommandPrinter wish will use the information
     * @param summarize enable a one line summary
     * @param unpack_param_data if the summary isn't enabled, this controls if
     * we unpack and display parameter data
     */
    virtual void Print(CommandPrinter *printer,
                       bool summarize,
                       bool unpack_param_data) const {
      printer->Print(this, summarize, unpack_param_data);
    }

    /**
     * @brief Write this RDMCommand to an OutputStream
     * @param stream is a pointer to an OutputStream
     */
    void Write(ola::io::OutputStream *stream) const;

    static const uint8_t START_CODE = 0xcc;

    static RDMCommand *Inflate(const uint8_t *data, unsigned int length);

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


/**
 * @brief RDM Commands that represent requests (GET, SET or DISCOVER).
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

    virtual void Print(CommandPrinter *printer,
                       bool summarize,
                       bool unpack_param_data) const {
      printer->Print(this, summarize, unpack_param_data);
    }

    // Convert a block of data to an RDMCommand object
    static RDMRequest* InflateFromData(const uint8_t *data,
                                       unsigned int length);
    static RDMRequest* InflateFromData(const string &data);

 private:
    RDMCommandClass m_command_class;
};


/**
 * @brief The parent class for GET/SET requests.
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


/**
 * @brief The set of RDM Commands that represent responses (GET, SET or
 * DISCOVER).
 */
class RDMResponse: public RDMCommand {
 public:
    RDMResponse(const UID &source,
                const UID &destination,
                uint8_t transaction_number,
                uint8_t response_type,
                uint8_t message_count,
                uint16_t sub_device,
                RDMCommand::RDMCommandClass command_class,
                uint16_t param_id,
                const uint8_t *data,
                unsigned int length)
          : RDMCommand(source, destination, transaction_number, response_type,
                       message_count, sub_device, param_id, data, length),
            m_command_class(command_class) {
    }

    uint8_t ResponseType() const { return m_port_id; }

    virtual void Print(CommandPrinter *printer,
                       bool summarize,
                       bool unpack_param_data) const {
      printer->Print(this, summarize, unpack_param_data);
    }

    RDMCommandClass CommandClass() const { return m_command_class; }

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

 private:
    RDMCommand::RDMCommandClass m_command_class;
};


/**
 * @brief The base class for GET/SET responses.
 */
class RDMGetSetResponse: public RDMResponse {
 public:
    RDMGetSetResponse(const UID &source,
                      const UID &destination,
                      uint8_t transaction_number,
                      uint8_t response_type,
                      uint8_t message_count,
                      uint16_t sub_device,
                      RDMCommand::RDMCommandClass command_class,
                      uint16_t param_id,
                      const uint8_t *data,
                      unsigned int length)
        : RDMResponse(source, destination, transaction_number, response_type,
                      message_count, sub_device, command_class, param_id, data,
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
                    unsigned int length)
        : RDMGetSetResponse(source, destination, transaction_number,
                            response_type, message_count, sub_device,
                            command_class, param_id, data, length) {
    }
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
                            rdm_nack_reason reason,
                            uint8_t outstanding_messages = 0);
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
 * @brief An RDM request of type DISCOVER_COMMAND.
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

    virtual void Print(CommandPrinter *printer,
                       bool summarize,
                       bool unpack_param_data) const {
      printer->Print(this, summarize, unpack_param_data);
    }

    static RDMDiscoveryRequest* InflateFromData(const uint8_t *data,
                                                unsigned int length);
    static RDMDiscoveryRequest* InflateFromData(const string &data);
};


// Because the number of discovery requests is small (3 type) we provide a
// helper method for each here.
/**
 * @brief Create a new DUB request object.
 */
RDMDiscoveryRequest *NewDiscoveryUniqueBranchRequest(
    const UID &source,
    const UID &lower,
    const UID &upper,
    uint8_t transaction_number,
    uint8_t port_id = 1);


/**
 * @brief Create a new Mute Request Object.
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
 * @brief An RDM response of type DISCOVER_COMMAND
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
                      DISCOVER_COMMAND_RESPONSE,
                      param_id,
                      data,
                      length) {
    }

    virtual void Print(CommandPrinter *printer,
                       bool summarize,
                       bool unpack_param_data) const {
      printer->Print(this, summarize, unpack_param_data);
    }

    static RDMDiscoveryResponse* InflateFromData(const uint8_t *data,
                                                 unsigned int length);
    static RDMDiscoveryResponse* InflateFromData(const string &data);
};
/** @} */
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RDMCOMMAND_H_
