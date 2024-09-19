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
 * The interface for the class which builds Message objects from a vector of
 * strings.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_MESSAGING_STRINGMESSAGEBUILDER_H_
#define INCLUDE_OLA_MESSAGING_STRINGMESSAGEBUILDER_H_

#include <ola/messaging/DescriptorVisitor.h>
#include <string>
#include <vector>

namespace ola {
namespace messaging {


/**
 * This visitor builds a Message object from a list of strings.
 */
class StringMessageBuilder: public FieldDescriptorVisitor {
 public:
    explicit StringMessageBuilder(const std::vector<std::string> &input)
        : m_input(input) {
    }
    ~StringMessageBuilder() {}

    void Visit(const BoolFieldDescriptor*);
    void Visit(const IPV4FieldDescriptor*);
    void Visit(const IPV6FieldDescriptor*);
    void Visit(const MACFieldDescriptor*);
    void Visit(const UIDFieldDescriptor*);
    void Visit(const StringFieldDescriptor*);
    void Visit(const IntegerFieldDescriptor<uint8_t>*);
    void Visit(const IntegerFieldDescriptor<uint16_t>*);
    void Visit(const IntegerFieldDescriptor<uint32_t>*);
    void Visit(const IntegerFieldDescriptor<uint64_t>*);
    void Visit(const IntegerFieldDescriptor<int8_t>*);
    void Visit(const IntegerFieldDescriptor<int16_t>*);
    void Visit(const IntegerFieldDescriptor<int32_t>*);
    void Visit(const IntegerFieldDescriptor<int64_t>*);
    void Visit(const FieldDescriptorGroup*);
    void PostVisit(const FieldDescriptorGroup*);

 private:
    std::vector<std::string> m_input;
};
}  // namespace messaging
}  // namespace ola
#endif  // INCLUDE_OLA_MESSAGING_STRINGMESSAGEBUILDER_H_
