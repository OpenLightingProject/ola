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
 * StringMessageBuilder.h
 * Builds a Message object from a list of strings & a Descriptor.
 * Copyright (C) 2011 Simon Newton
 */

/**
 * @addtogroup rdm_helpers
 * @{
 * @file include/ola/rdm/StringMessageBuilder.h
 * @brief Builds a Message object from a list of strings and a Descriptor.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_STRINGMESSAGEBUILDER_H_
#define INCLUDE_OLA_RDM_STRINGMESSAGEBUILDER_H_

#include <ola/messaging/DescriptorVisitor.h>
#include <stack>
#include <string>
#include <vector>

namespace ola {

namespace messaging {
class Descriptor;
class MessageFieldInterface;
class Message;
}

namespace rdm {

/**
 * This visitor builds a message based on a vector of strings from a
 * Descriptor.
 */
class StringMessageBuilder: public ola::messaging::FieldDescriptorVisitor {
 public:
    StringMessageBuilder();
    ~StringMessageBuilder();

    // we handle descending into groups ourself
    bool Descend() const { return false; }
    const ola::messaging::Message *GetMessage(
        const std::vector<std::string> &inputs,
        const class ola::messaging::Descriptor *descriptor);
    const std::string GetError() const { return m_error_string; }

    void Visit(const ola::messaging::BoolFieldDescriptor*);
    void Visit(const ola::messaging::IPV4FieldDescriptor*);
    void Visit(const ola::messaging::MACFieldDescriptor*);
    void Visit(const ola::messaging::UIDFieldDescriptor*);
    void Visit(const ola::messaging::StringFieldDescriptor*);
    void Visit(const ola::messaging::UInt8FieldDescriptor*);
    void Visit(const ola::messaging::UInt16FieldDescriptor*);
    void Visit(const ola::messaging::UInt32FieldDescriptor*);
    void Visit(const ola::messaging::Int8FieldDescriptor*);
    void Visit(const ola::messaging::Int16FieldDescriptor*);
    void Visit(const ola::messaging::Int32FieldDescriptor*);
    void Visit(const ola::messaging::FieldDescriptorGroup*);
    void PostVisit(const ola::messaging::FieldDescriptorGroup*);

 private:
    std::vector<std::string> m_inputs;
    std::stack<
        std::vector<const ola::messaging::MessageFieldInterface*> > m_groups;
    unsigned int m_offset, m_input_size, m_group_instance_count;
    bool m_error;
    std::string m_error_string;

    bool StopParsing() const;
    void SetError(const std::string &error);

    template<typename type>
    void VisitInt(const ola::messaging::IntegerFieldDescriptor<type> *);

    void InitVars(const std::vector<std::string> &inputs);
    void CleanUpVector();
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_STRINGMESSAGEBUILDER_H_
