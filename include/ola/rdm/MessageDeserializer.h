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
 * MessageDeserializer.h
 * Inflate a message from raw data.
 * Copyright (C) 2011 Simon Newton
 */

/**
 * @addtogroup rdm_command
 * @{
 * @file MessageDeserializer.h
 * @brief Inflate a message from raw data
 * @}
 *
 */

#ifndef INCLUDE_OLA_RDM_MESSAGEDESERIALIZER_H_
#define INCLUDE_OLA_RDM_MESSAGEDESERIALIZER_H_

#include <ola/messaging/DescriptorVisitor.h>
#include <ola/messaging/Message.h>
#include <stack>
#include <vector>

namespace ola {
namespace rdm {


/**
 * This visitor inflates the message from raw data.
 */
class MessageDeserializer: public ola::messaging::FieldDescriptorVisitor {
 public:
    MessageDeserializer();
    ~MessageDeserializer();

    const ola::messaging::Message *InflateMessage(
        const class ola::messaging::Descriptor *descriptor,
        const uint8_t *data,
        unsigned int length);

    // we handle descending into groups ourself
    bool Descend() const { return false; }

    void Visit(const ola::messaging::BoolFieldDescriptor*);
    void Visit(const ola::messaging::IPV4FieldDescriptor*);
    void Visit(const ola::messaging::IPV6FieldDescriptor*);
    void Visit(const ola::messaging::MACFieldDescriptor*);
    void Visit(const ola::messaging::UIDFieldDescriptor*);
    void Visit(const ola::messaging::StringFieldDescriptor*);
    void Visit(const ola::messaging::IntegerFieldDescriptor<uint8_t>*);
    void Visit(const ola::messaging::IntegerFieldDescriptor<uint16_t>*);
    void Visit(const ola::messaging::IntegerFieldDescriptor<uint32_t>*);
    void Visit(const ola::messaging::IntegerFieldDescriptor<uint64_t>*);
    void Visit(const ola::messaging::IntegerFieldDescriptor<int8_t>*);
    void Visit(const ola::messaging::IntegerFieldDescriptor<int16_t>*);
    void Visit(const ola::messaging::IntegerFieldDescriptor<int32_t>*);
    void Visit(const ola::messaging::IntegerFieldDescriptor<int64_t>*);
    void Visit(const ola::messaging::FieldDescriptorGroup*);
    void PostVisit(const ola::messaging::FieldDescriptorGroup*) {}

 private:
    const uint8_t *m_data;
    unsigned int m_length;
    unsigned int m_offset;
    unsigned int m_variable_field_size;
    bool m_insufficient_data;

    typedef std::vector<const ola::messaging::MessageFieldInterface*>
        message_vector;
    std::stack<message_vector> m_message_stack;

    bool CheckForData(unsigned int required_size);
    void CleanUpVector();

    template <typename int_type>
    void IntVisit(const ola::messaging::IntegerFieldDescriptor<int_type> *);
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_MESSAGEDESERIALIZER_H_
