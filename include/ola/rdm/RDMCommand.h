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
 * RDMCommand.h
 * All the classes that represent RDM commands.
 * Copyright (C) 2005 Simon Newton
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
#include <ola/base/Macro.h>
#include <ola/io/ByteString.h>
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
 * @brief The base class that all RDM requests & responses inherit from.
 *
 * @note RDMCommands may hold more than 231 bytes of data. Use the
 * RDMCommandSerializer class if you want the wire format.
 */
class RDMCommand {
 public:
  /**
   * @brief A set of values representing CommandClasses in E1.20.
   * @note Please see section 6.2.10 of ANSI E1.20 for more information.
   */
  // TODO(simon): remove this in favor of the new one in RDMEnums.h
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
   * @name String Methods
   * @{
   */

  /**
   * @brief Create a human readable string from the RDMCommand object.
   * @returns A string containing the source and destination UIDs, transaction
   * number, port ID, Message count, Sub Device, Cmd Class, Param ID, Data,
   * and a raw string of the parameter data.
   */
  std::string ToString() const;

  /**
   * @brief Output an RDMCommand object to an ostream.
   * @param out ostream to output to
   * @param command is the RDMCommand to print
   * @sa ToString()
   */
  friend std::ostream& operator<<(std::ostream &out,
                                  const RDMCommand &command) {
    return out << command.ToString();
  }

  /** @} */

  /**
   * @name Accessors
   * @{
   */

  /** @brief The Sub-Start code for the RDMCommand */
  virtual uint8_t SubStartCode() const { return SUB_START_CODE; }

  /** @brief The Message length field. */
  virtual uint8_t MessageLength() const;

  /** @brief Returns the Source UID of the RDMCommand */
  const UID& SourceUID() const { return m_source; }

  /** @brief Returns the Destination UID of the RDMCommand */
  const UID& DestinationUID() const { return m_destination; }

  /** @brief Returns the Transaction Number of the RDMCommand */
  uint8_t TransactionNumber() const { return m_transaction_number; }

  /** @brief Returns the Port ID of the RDMCommand */
  uint8_t PortIdResponseType() const { return m_port_id; }

  /** @brief Returns the Message Count of the RDMCommand */
  uint8_t MessageCount() const { return m_message_count; }

  /** @brief Returns the SubDevice of the RDMCommand */
  uint16_t SubDevice() const { return m_sub_device; }

  /** @brief The CommmandClass of this message */
  virtual RDMCommandClass CommandClass() const = 0;

  /** @brief Returns the Parameter ID of the RDMCommand */
  uint16_t ParamId() const { return m_param_id; }

  /** @brief Returns the Size of the Parameter Data of the RDMCommand */
  unsigned int ParamDataSize() const { return m_data_length; }

  /** @brief Returns the Parameter Data of the RDMCommand */
  const uint8_t *ParamData() const { return m_data; }

  /** @} */

  /**
   * @brief Modify the calculated checksum for this command.
   * @param checksum The original calculated checksum of the command.
   * @returns The new checksum to use for the command.
   *
   * This can be used to generate commands with invalid checksums.
   */
  virtual uint16_t Checksum(uint16_t checksum) const { return checksum; }

  /**
   * @brief Output the contents of the command to a CommandPrinter.
   * @param printer CommandPrinter which will use the information
   * @param summarize enable a one line summary
   * @param unpack_param_data if the summary isn't enabled, this controls if
   * we unpack and display parameter data.
   */
  virtual void Print(CommandPrinter *printer,
                     bool summarize,
                     bool unpack_param_data) const {
    printer->Print(this, summarize, unpack_param_data);
  }

  /**
   * @brief Test for equality.
   * @param other The RDMCommand to test against.
   * @returns True if two RDMCommands are equal.
   */
  bool operator==(const RDMCommand &other) const;

  /**
   * @brief The RDM Start Code.
   */
  static const uint8_t START_CODE = 0xcc;

  /**
   * @brief Extract a RDMCommand from raw data.
   * @param data The data excluding the state code.
   * @param length The length of the data.
   * @returns NULL if the RDM command is invalid.
   */
  static RDMCommand *Inflate(const uint8_t *data, unsigned int length);

 protected:
  uint8_t m_port_id;
  UID m_source;
  UID m_destination;
  uint8_t m_transaction_number;

  /**
   * @brief Protected constructor for derived classes.
   */
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

  static RDMStatusCode VerifyData(const uint8_t *data,
                                  size_t length,
                                  RDMCommandHeader *command_message);

  static RDMCommandClass ConvertCommandClass(uint8_t command_type);

 private:
  uint8_t m_message_count;
  uint16_t m_sub_device;
  uint16_t m_param_id;
  uint8_t *m_data;
  unsigned int m_data_length;

  static uint16_t CalculateChecksum(const uint8_t *data,
                                    unsigned int packet_length);

  DISALLOW_COPY_AND_ASSIGN(RDMCommand);
};


/**
 * @brief RDM Commands that represent requests (GET, SET or DISCOVER).
 */
class RDMRequest: public RDMCommand {
 public:
  struct OverrideOptions {
   public:
    /**
     * @brief Allow all fields in a RDMRequest to be specified. Using values
     * other than the default may result in invalid RDM messages.
     */
    OverrideOptions()
      : has_message_length(false),
        has_checksum(false),
        sub_start_code(SUB_START_CODE),
        message_length(0),
        message_count(0),
        checksum(0) {
    }

    void SetMessageLength(uint8_t message_length_arg) {
      has_message_length = true;
      message_length = message_length_arg;
    }

    void SetChecksum(uint16_t checksum_arg) {
      has_checksum = true;
      checksum = checksum_arg;
    }

    bool has_message_length;
    bool has_checksum;

    uint8_t sub_start_code;
    uint8_t message_length;
    uint8_t message_count;
    uint16_t checksum;
  };

  /**
   * @brief Create a new request.
   * @param source The source UID.
   * @param destination The destination UID.
   * @param transaction_number The transaction number.
   * @param port_id The Port ID.
   * @param sub_device The Sub Device index.
   * @param command_class The Command Class of this request.
   * @param param_id The PID value.
   * @param data The parameter data, or NULL if there isn't any.
   * @param length The length of the parameter data.
   * @param options The OverrideOptions.
   */
  RDMRequest(const UID &source,
             const UID &destination,
             uint8_t transaction_number,
             uint8_t port_id,
             uint16_t sub_device,
             RDMCommandClass command_class,
             uint16_t param_id,
             const uint8_t *data,
             unsigned int length,
             const OverrideOptions &options = OverrideOptions());

  RDMCommandClass CommandClass() const { return m_command_class; }

  /**
   * @brief The Port ID for this request.
   * @returns The Port ID.
   */
  uint8_t PortId() const { return m_port_id; }

  /**
   * @brief Make a copy of the request.
   * @returns A new RDMRequest that is identical to this one.
   */
  virtual RDMRequest *Duplicate() const {
    return new RDMRequest(
      SourceUID(),
      DestinationUID(),
      TransactionNumber(),
      PortId(),
      SubDevice(),
      m_command_class,
      ParamId(),
      ParamData(),
      ParamDataSize(),
      m_override_options);
  }

  virtual void Print(CommandPrinter *printer,
                     bool summarize,
                     bool unpack_param_data) const {
    printer->Print(this, summarize, unpack_param_data);
  }

  /**
   * @brief Check if this is a DUB request.
   * @returns true if this is a DUB request.
   */
  bool IsDUB() const;

  uint8_t SubStartCode() const;
  uint8_t MessageLength() const;
  uint16_t Checksum(uint16_t checksum) const;

  /**
   * @name Mutators
   * @{
   */

  /**
   * @brief Set the source UID
   * @param source_uid The new source UID.
   */
  void SetSourceUID(const UID &source_uid) {
    m_source = source_uid;
  }

  /**
   * @brief Set the transaction number.
   * @param transaction_number the new transaction number.
   */
  void SetTransactionNumber(uint8_t transaction_number) {
    m_transaction_number = transaction_number;
  }

  /**
   * @brief Set the Port Id.
   * @param port_id the new Port Id.
   */
  void SetPortId(uint8_t port_id) {
    m_port_id = port_id;
  }

  /** @} */

  /**
   * @brief Inflate a request from some data.
   * @param data The raw data.
   * @param length The length of the data.
   * @returns A RDMRequest object or NULL if the data was invalid.
   */
  static RDMRequest* InflateFromData(const uint8_t *data,
                                     unsigned int length);

 protected:
  OverrideOptions m_override_options;

 private:
  RDMCommandClass m_command_class;
};


/**
 * @brief An RDM Get / Set Request.
 */
class RDMGetSetRequest: public RDMRequest {
 public:
  /**
   * @brief Create a new Get / Set Request.
   * @param source The source UID.
   * @param destination The destination UID.
   * @param transaction_number The transaction number.
   * @param port_id The Port ID.
   * @param sub_device The Sub Device index.
   * @param command_class The Command Class of this request.
   * @param param_id The PID value.
   * @param data The parameter data, or NULL if there isn't any.
   * @param length The length of the parameter data.
   * @param options The OverrideOptions.
   */
  RDMGetSetRequest(const UID &source,
                   const UID &destination,
                   uint8_t transaction_number,
                   uint8_t port_id,
                   uint16_t sub_device,
                   RDMCommandClass command_class,
                   uint16_t param_id,
                   const uint8_t *data,
                   unsigned int length,
                   const OverrideOptions &options)
      : RDMRequest(source, destination, transaction_number, port_id,
                   sub_device, command_class, param_id, data, length, options) {
  }
};


template <RDMCommand::RDMCommandClass command_class>
class BaseRDMRequest: public RDMGetSetRequest {
 public:
  BaseRDMRequest(const UID &source,
                 const UID &destination,
                 uint8_t transaction_number,
                 uint8_t port_id,
                 uint16_t sub_device,
                 uint16_t param_id,
                 const uint8_t *data,
                 unsigned int length,
                 const OverrideOptions &options = OverrideOptions())
    : RDMGetSetRequest(source, destination, transaction_number, port_id,
                       sub_device, command_class, param_id, data, length,
                       options) {
  }

  BaseRDMRequest<command_class> *Duplicate() const {
    return new BaseRDMRequest<command_class>(
      SourceUID(),
      DestinationUID(),
      TransactionNumber(),
      PortId(),
      SubDevice(),
      ParamId(),
      ParamData(),
      ParamDataSize(),
      m_override_options);
  }
};

typedef BaseRDMRequest<RDMCommand::GET_COMMAND> RDMGetRequest;
typedef BaseRDMRequest<RDMCommand::SET_COMMAND> RDMSetRequest;


/**
 * @brief An RDM Command that represents responses (GET, SET or
 * DISCOVER).
 */
class RDMResponse: public RDMCommand {
 public:
  /**
   * @brief Create a new RDM Response.
   * @param source The source UID.
   * @param destination The destination UID.
   * @param transaction_number The transaction number.
   * @param response_type The Response Type.
   * @param message_count Set to 0.
   * @param sub_device The Sub Device index.
   * @param command_class The Command Class of this request.
   * @param param_id The PID value.
   * @param data The parameter data, or NULL if there isn't any.
   * @param length The length of the parameter data.
   */
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

  virtual void Print(CommandPrinter *printer,
                     bool summarize,
                     bool unpack_param_data) const {
    printer->Print(this, summarize, unpack_param_data);
  }

  /**
   * @brief Make a copy of the response.
   * @returns A new RDMResponse that is identical to this one.
   */
  RDMResponse *Duplicate() const {
    return new RDMResponse(
      SourceUID(),
      DestinationUID(),
      TransactionNumber(),
      ResponseType(),
      MessageCount(),
      SubDevice(),
      CommandClass(),
      ParamId(),
      ParamData(),
      ParamDataSize());
  }

  /**
   * @name Accessors
   * @{
   */

  /**
   * @brief The Response Type.
   * @returns The Response Type (ACK, NACK, etc.)
   */
  uint8_t ResponseType() const { return m_port_id; }

  RDMCommandClass CommandClass() const { return m_command_class; }

  /** @} */

  /**
   * @name Mutators
   * @{
   */

  /**
   * @brief Set the destination UID
   * @param destination_uid The new destination UID.
   */
  void SetDestinationUID(const UID &destination_uid) {
    m_destination = destination_uid;
  }

  /**
   * @brief Set the transaction number.
   * @param transaction_number the new transaction number.
   */
  void SetTransactionNumber(uint8_t transaction_number) {
    m_transaction_number = transaction_number;
  }

  /** @} */

  /**
   * @brief The maximum size of an ACK_OVERFLOW session that we'll buffer.
   *
   * 4k should be big enough for everyone ;)
   */
  static const unsigned int MAX_OVERFLOW_SIZE = 4 << 10;

  /**
   * Create a RDMResponse request from raw data.
   * @param data the response data.
   * @param length the length of the response data.
   * @param[out] status_code a pointer to a RDMStatusCode to set
   * @param request an optional RDMRequest object that this response is for
   * @returns a new RDMResponse object, or NULL is this response is invalid
   */
  static RDMResponse* InflateFromData(const uint8_t *data,
                                      size_t length,
                                      RDMStatusCode *status_code,
                                      const RDMRequest *request = NULL);

  /**
   * Create a RDMResponse request from raw data in a ByteString.
   * @param input the raw response data.
   * @param[out] status_code a pointer to a RDMStatusCode to set
   * @param request an optional RDMRequest object that this response is for
   * @returns a new RDMResponse object, or NULL is this response is invalid
   */
  static RDMResponse* InflateFromData(const ola::io::ByteString &input,
                                      RDMStatusCode *status_code,
                                      const RDMRequest *request = NULL) {
    return InflateFromData(input.data(), input.size(), status_code, request);
  }

  /**
   * @brief Combine two RDMResponses.
   * @param response1 the first response.
   * @param response2 the second response.
   * @return A new response with the data from the first and second combined or
   * NULL if the size limit is reached.
   *
   * This is used to combine the data from two responses in an ACK_OVERFLOW
   * session.
   */
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

/**
 * @brief Generate a NACK response with a reason code
 */
RDMResponse *NackWithReason(const RDMRequest *request,
                            rdm_nack_reason reason,
                            uint8_t outstanding_messages = 0);
/**
 * @brief Generate an ACK Response with some data
 */
RDMResponse *GetResponseFromData(const RDMRequest *request,
                                 const uint8_t *data = NULL,
                                 unsigned int length = 0,
                                 rdm_response_type type = RDM_ACK,
                                 uint8_t outstanding_messages = 0);

/**
 * @brief Construct an RDM response from a RDMRequest object.
 */
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
                        uint16_t sub_device,
                        uint16_t param_id,
                        const uint8_t *data,
                        unsigned int length,
                        const OverrideOptions &options = OverrideOptions())
        : RDMRequest(source,
                     destination,
                     transaction_number,
                     port_id,
                     sub_device,
                     DISCOVER_COMMAND,
                     param_id,
                     data,
                     length,
                     options) {
    }

    uint8_t PortId() const { return m_port_id; }

    virtual void Print(CommandPrinter *printer,
                       bool summarize,
                       bool unpack_param_data) const {
      printer->Print(this, summarize, unpack_param_data);
    }

    static RDMDiscoveryRequest* InflateFromData(const uint8_t *data,
                                                unsigned int length);
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
};
/** @} */
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RDMCOMMAND_H_
