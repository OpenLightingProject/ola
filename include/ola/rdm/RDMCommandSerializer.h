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
#include <ola/io/ByteString.h>
#include <ola/io/IOStack.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/UID.h>

namespace ola {
namespace rdm {


/**
 * @brief Serializes RDMCommands.
 *
 * This creates the binary representation of an RDMCommand. The binary
 * representation is restricted to 231 bytes of parameter data. If
 * the message is more than 231 bytes then the methods will return false.
 */
class RDMCommandSerializer {
 public:
  /**
   * @brief Serialize a RDMCommand to a ByteString, without the RDM Start Code.
   * @param command the RDMCommand to serialize.
   * @param[out] output The ByteString to append to.
   * @returns True if the command was serialized correctly, false otherwise.
   */
  static bool Pack(const RDMCommand &command,
                   ola::io::ByteString *output);

  /**
   * @brief Serialize a RDMCommand to a ByteString, with the RDM Start Code.
   * @param command the RDMCommand to serialize.
   * @param[out] output The ByteString to append to.
   * @returns True if the command was serialized correctly, false otherwise.
   */
  static bool PackWithStartCode(const RDMCommand &command,
                                ola::io::ByteString *output);

  /**
   * @brief Return the number of bytes required to store the serialized version
   *   of the RDMCommand.
   * @param command The RDMCommand which will be serialized.
   * @returns The number of bytes required for the serialized form of the
   * command or 0 if the command contains more than 231 bytes of parameter data.
   */
  static unsigned int RequiredSize(const RDMCommand &command);

  /**
   * @brief Serialize a RDMCommand to an array of bytes.
   * @param command the RDMCommand to serialize.
   * @param buffer The memory location to serialize to.
   * @param[in,out] size The size of the memory location.
   * @returns True if the command was serialized correctly, false otherwise.
   *
   * The size of the memory location should be at least as large as what was
   * returned from RequiredSize().
   */
  static bool Pack(const RDMCommand &command,
                   uint8_t *buffer,
                   unsigned int *size);

  // TODO(simon): Add IOQueue Write() method here

  /**
   * @brief Write the binary representation of an RDMCommand to an IOStack.
   * @param command the RDMCommand
   * @param stack the IOStack to write to.
   * @returns true if the write was successful, false if the RDM command needs
   *   to be fragmented.
   */
  static bool Write(const RDMCommand &command, ola::io::IOStack *stack);

  /**
   * @brief The maximum parameter data a single command can contain.
   */
  enum { MAX_PARAM_DATA_LENGTH = 231 };

 private:
  static const unsigned int CHECKSUM_LENGTH = 2;

  static void PopulateHeader(RDMCommandHeader *header,
                             const RDMCommand &command);
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RDMCOMMANDSERIALIZER_H_
