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
 * MessageVisitor.h
 * The interface for the class which visits Message & MessageFields
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_MESSAGING_MESSAGEVISITOR_H_
#define INCLUDE_OLA_MESSAGING_MESSAGEVISITOR_H_

#include <stdint.h>

namespace ola {
namespace messaging {


class BoolMessageField;
class GroupMessageField;
class IPV4MessageField;
class IPV6MessageField;
class MACMessageField;
class StringMessageField;
class UIDMessageField;

template <typename type>
class BasicMessageField;

/**
 * The interface for the Message Visitor
 */
class MessageVisitor {
 public:
    virtual ~MessageVisitor() {}

    virtual void Visit(const BoolMessageField*) = 0;
    virtual void Visit(const IPV4MessageField*) = 0;
    virtual void Visit(const IPV6MessageField*) = 0;
    virtual void Visit(const MACMessageField*) = 0;
    virtual void Visit(const UIDMessageField*) = 0;
    virtual void Visit(const StringMessageField*) = 0;
    virtual void Visit(const BasicMessageField<uint8_t>*) = 0;
    virtual void Visit(const BasicMessageField<uint16_t>*) = 0;
    virtual void Visit(const BasicMessageField<uint32_t>*) = 0;
    virtual void Visit(const BasicMessageField<uint64_t>*) = 0;
    virtual void Visit(const BasicMessageField<int8_t>*) = 0;
    virtual void Visit(const BasicMessageField<int16_t>*) = 0;
    virtual void Visit(const BasicMessageField<int32_t>*) = 0;
    virtual void Visit(const BasicMessageField<int64_t>*) = 0;
    virtual void Visit(const GroupMessageField*) = 0;
    virtual void PostVisit(const GroupMessageField*) = 0;
};
}  // namespace messaging
}  // namespace ola
#endif  // INCLUDE_OLA_MESSAGING_MESSAGEVISITOR_H_
