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
 * RDMCommandSerializer.h
 * Write RDMCommands to a memory buffer.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_RDMCOMMANDSERIALIZER_H_
#define INCLUDE_OLA_RDM_RDMCOMMANDSERIALIZER_H_

#include <stdint.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/UID.h>

namespace ola {
namespace rdm {


/*
 * Serializes a RDMCommand to a memory buffer. This restricts the serialized
 * command to 231 butes of paramater data. If the message is more than 231
 * bytes  then Pack will return false.
 */
class RDMCommandSerializer {
  public:
    static unsigned int RequiredSize(const RDMCommand &command);

    static bool Pack(const RDMCommand &command,
              uint8_t *buffer,
              unsigned int *size);
    static bool Pack(const RDMRequest &request,
              uint8_t *buffer,
              unsigned int *size,
              const UID &source,
              uint8_t transaction_number,
              uint8_t port_id);

    enum { MAX_PARAM_DATA_LENGTH = 231 };

  private:
    static const unsigned int CHECKSUM_LENGTH = 2;

    static bool PackWithParams(const RDMCommand &request,
                               uint8_t *buffer,
                               unsigned int *size,
                               const UID &source,
                               uint8_t transaction_number,
                               uint8_t port_id);
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_RDMCOMMANDSERIALIZER_H_
