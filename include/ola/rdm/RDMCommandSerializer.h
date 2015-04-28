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

/**
 * @addtogroup rdm_command
 * @{
 * @file RDMCommandSerializer.h
 * @brief Write RDMCommands to a memory buffer.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_RDMCOMMANDSERIALIZER_H_
#define INCLUDE_OLA_RDM_RDMCOMMANDSERIALIZER_H_

#include <stdint.h>
#include <ola/io/IOStack.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/UID.h>

namespace ola {
namespace rdm {


/**
 * @brief Serializes RDMCommands.
 *
 * This creates the binary representation of an RDMCommand. The binary
 * representation is restricted to 231 bytes of paramater data. If
 * the message is more than 231 bytes then Pack() will return false.
 */
class RDMCommandSerializer {
 public:
  /**
   * @brief Return the number of bytes required to store the serialized version
   * of the RDMCommand.
   * @param command The RDMCommand which will be serialized.
   * @returns The number of bytes required for the serialized form of the
   * command or 0 if the command contains more than 231 bytes of parameter data.
   */
  static unsigned int RequiredSize(const RDMCommand &command);

  /**
   * @brief Serialize a RDMCommand to an array of bytes.
   * @param command the RDMCommand to serialize.
   * @param buffer The memory location to serailize to.
   * @param[in,out] size The size of the memory location.
   * @returns True if the command was serialized correctly, false otherwise.
   *
   * The size of the memory location should be at least as large as what was
   * returned from RequiredSize().
   */
  static bool Pack(const RDMCommand &command,
                   uint8_t *buffer,
                   unsigned int *size);

  /**
   * @brief Serialize a RDMCommand to an array of bytes.
   * @param request the RDMRequest to serialize.
   * @param buffer The memory location to serailize to.
   * @param[in,out] size The size of the memory location.
   * @param source The new Source UID.
   * @param transaction_number The new transaction number.
   * @param port_id The new Port Id.
   * @returns True if the command was serialized correctly, false otherwise.
   * @deprecated Use the mutator methods of RDMCommand instead.
   *
   * The size of the memory location should be at least as large as what was
   * returned from RequiredSize().
   *
   */
  static bool Pack(const RDMRequest &request,
                   uint8_t *buffer,
                   unsigned int *size,
                   const UID &source,
                   uint8_t transaction_number,
                   uint8_t port_id);

  // TODO(simon): Add IOQueue Write() methods here

  /**
   * @brief Write the binary representation of an RDMCommand to an IOStack.
   * @param command the RDMCommand
   * @param stack the IOStack to write to.
   * @returns true if the write was successful, false if the RDM command needs
   *   to be fragmented.
   */
  static bool Write(const RDMCommand &command, ola::io::IOStack *stack);

  /**
   * @brief Write a RDMRequest to an IOStack as a RDM message.
   * @param request the RDMRequest
   * @param stack the IOStack to write to.
   * @param source the source UID
   * @param transaction_number the RDM transaction number
   * @param port_id the RDM port id
   * @returns true if the write was successful, false if the RDM request needs
   *   to be fragmented.
   * @deprecated Use the mutator methods of RDMCommand instead.
   */
  static bool Write(const RDMRequest &request,
                    ola::io::IOStack *stack,
                    const UID &source,
                    uint8_t transaction_number,
                    uint8_t port_id);

  /**
   * @brief The maximum parameter data a single command can contain.
   */
  enum { MAX_PARAM_DATA_LENGTH = 231 };

 private:
  static const unsigned int CHECKSUM_LENGTH = 2;

  static bool PackWithParams(const RDMCommand &command,
                             uint8_t *buffer,
                             unsigned int *size,
                             const UID &source,
                             uint8_t transaction_number,
                             uint8_t port_id);

  static bool WriteToStack(const RDMCommand &command,
                           ola::io::IOStack *stack,
                           const UID &source,
                           uint8_t transaction_number,
                           uint8_t port_id);

  static void PopulateHeader(RDMCommandHeader *header,
                             const RDMCommand &command,
                             unsigned int packet_length,
                             const UID &source,
                             uint8_t transaction_number,
                             uint8_t port_id);
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RDMCOMMANDSERIALIZER_H_
