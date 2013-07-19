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
 * StreamRpcHeader.h
 * The header for the RPC messages.
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef COMMON_RPC_STREAMRPCHEADER_H_
#define COMMON_RPC_STREAMRPCHEADER_H_

#include <stdint.h>

namespace ola {
namespace rpc {

class StreamRpcHeader {
  /*
   * The first 4 bytes are the header which contains the RPC protocol version
   * (this is separate from the protobuf version) and the size of the protobuf.
   */
  public:
    /**
     * Encode a header
     */
    static void EncodeHeader(uint32_t *header, unsigned int version,
                             unsigned int size) {
      *header = (version << 28) & VERSION_MASK;
      *header |= size & SIZE_MASK;
    }

    /**
     * Decode a header
     */
    static void DecodeHeader(uint32_t header, unsigned int *version,
                             unsigned int *size) {
      *version = (header & VERSION_MASK) >> 28;
      *size = header & SIZE_MASK;
    }

  private:
    static const unsigned int VERSION_MASK = 0xf0000000;
    static const unsigned int SIZE_MASK = 0x0fffffff;
};
}  // namespace rpc
}  // namespace ola
#endif  // COMMON_RPC_STREAMRPCHEADER_H_
