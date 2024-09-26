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
 * MessageSerializer.h
 * Serialize a message.
 * Copyright (C) 2011 Simon Newton
 */

/**
 * @addtogroup rdm_command
 * @{
 * @file MessageSerializer.h
 * @brief Serialize an RDM message.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_MESSAGESERIALIZER_H_
#define INCLUDE_OLA_RDM_MESSAGESERIALIZER_H_

#include <ola/messaging/MessageVisitor.h>

namespace ola {
namespace rdm {


/**
 * This visitor serializes the message.
 */
class MessageSerializer: public ola::messaging::MessageVisitor {
 public:
    explicit MessageSerializer(unsigned int initial_size = INITIAL_BUFFER_SIZE);
    ~MessageSerializer();

    const uint8_t *SerializeMessage(const ola::messaging::Message *message,
                                    unsigned int *length);

    void Visit(const ola::messaging::BoolMessageField*);
    void Visit(const ola::messaging::IPV4MessageField*);
    void Visit(const ola::messaging::IPV6MessageField*);
    void Visit(const ola::messaging::MACMessageField*);
    void Visit(const ola::messaging::UIDMessageField*);
    void Visit(const ola::messaging::StringMessageField*);
    void Visit(const ola::messaging::BasicMessageField<uint8_t>*);
    void Visit(const ola::messaging::BasicMessageField<uint16_t>*);
    void Visit(const ola::messaging::BasicMessageField<uint32_t>*);
    void Visit(const ola::messaging::BasicMessageField<uint64_t>*);
    void Visit(const ola::messaging::BasicMessageField<int8_t>*);
    void Visit(const ola::messaging::BasicMessageField<int16_t>*);
    void Visit(const ola::messaging::BasicMessageField<int32_t>*);
    void Visit(const ola::messaging::BasicMessageField<int64_t>*);
    void Visit(const ola::messaging::GroupMessageField*);
    void PostVisit(const ola::messaging::GroupMessageField*);

 private:
    uint8_t *m_data;
    unsigned int m_offset, m_buffer_size, m_initial_buffer_size;

    static const unsigned int INITIAL_BUFFER_SIZE = 256;

    void CheckForFreeSpace(unsigned int required_size);

    template <typename int_type>
    void IntVisit(const ola::messaging::BasicMessageField<int_type> *);
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_MESSAGESERIALIZER_H_
