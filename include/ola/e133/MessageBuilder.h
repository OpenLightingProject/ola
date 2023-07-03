/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * MessageBuilder.h
 * Copyright (C) 2013 Simon Newton
 *
 * A class to simplify some of the E1.33 packet building operations.
 */

#ifndef INCLUDE_OLA_E133_MESSAGEBUILDER_H_
#define INCLUDE_OLA_E133_MESSAGEBUILDER_H_

#include <ola/acn/CID.h>
#include <ola/base/Macro.h>
#include <ola/e133/E133Enums.h>
#include <ola/io/IOStack.h>
#include <ola/io/MemoryBlockPool.h>
#include <string>

namespace ola {
namespace e133 {

using ola::acn::CID;
using ola::io::IOStack;
using std::string;

/**
 * Provides helper methods for common E1.33 packet construction operations.
 */
class MessageBuilder {
 public:
    MessageBuilder(const CID &cid, const string &source_name);
    ~MessageBuilder() {}

    void PrependRDMHeader(IOStack *packet);

    void BuildNullTCPPacket(IOStack *packet);

    void BuildBrokerNullTCPPacket(IOStack *packet);

    void BuildTCPE133StatusPDU(IOStack *packet,
                               uint32_t sequence_number, uint16_t endpoint_id,
                               ola::e133::E133StatusCode status_code,
                               const string &description);
    void BuildUDPE133StatusPDU(IOStack *packet,
                               uint32_t sequence_number, uint16_t endpoint_id,
                               ola::e133::E133StatusCode status_code,
                               const string &description);

    void BuildTCPRootE133(IOStack *packet, uint32_t vector,
                          uint32_t sequence_number, uint16_t endpoint_id);
    void BuildUDPRootE133(IOStack *packet, uint32_t vector,
                          uint32_t sequence_number, uint16_t endpoint_id);

    ola::io::MemoryBlockPool *pool() { return &m_memory_pool; }

 private:
    const CID m_cid;
    const string m_source_name;
    ola::io::MemoryBlockPool m_memory_pool;

    DISALLOW_COPY_AND_ASSIGN(MessageBuilder);
};
}  // namespace e133
}  // namespace ola
#endif  // INCLUDE_OLA_E133_MESSAGEBUILDER_H_
