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
 * DescriptorVisitor.h
 * The interface for the class which visits Descriptors & FieldDescriptors.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_MESSAGING_DESCRIPTORVISITOR_H_
#define INCLUDE_OLA_MESSAGING_DESCRIPTORVISITOR_H_

#include <stdint.h>

namespace ola {
namespace messaging {


class BoolFieldDescriptor;
class FieldDescriptorGroup;
class IPV4FieldDescriptor;
class IPV6FieldDescriptor;
class MACFieldDescriptor;
class StringFieldDescriptor;
class UIDFieldDescriptor;

template <typename type>
class IntegerFieldDescriptor;

/**
 * The interface for the FieldDescriptor Visitor
 */
class FieldDescriptorVisitor {
 public:
    virtual ~FieldDescriptorVisitor() {}

    // return true if you want groups to be recursively expanded
    virtual bool Descend() const = 0;

    virtual void Visit(const BoolFieldDescriptor*) = 0;
    virtual void Visit(const IPV4FieldDescriptor*) = 0;
    virtual void Visit(const IPV6FieldDescriptor*) = 0;
    virtual void Visit(const MACFieldDescriptor*) = 0;
    virtual void Visit(const UIDFieldDescriptor*) = 0;
    virtual void Visit(const StringFieldDescriptor*) = 0;
    virtual void Visit(const IntegerFieldDescriptor<uint8_t>*) = 0;
    virtual void Visit(const IntegerFieldDescriptor<uint16_t>*) = 0;
    virtual void Visit(const IntegerFieldDescriptor<uint32_t>*) = 0;
    virtual void Visit(const IntegerFieldDescriptor<uint64_t>*) = 0;
    virtual void Visit(const IntegerFieldDescriptor<int8_t>*) = 0;
    virtual void Visit(const IntegerFieldDescriptor<int16_t>*) = 0;
    virtual void Visit(const IntegerFieldDescriptor<int32_t>*) = 0;
    virtual void Visit(const IntegerFieldDescriptor<int64_t>*) = 0;
    virtual void Visit(const FieldDescriptorGroup*) = 0;
    virtual void PostVisit(const FieldDescriptorGroup*) = 0;
};
}  // namespace messaging
}  // namespace ola
#endif  // INCLUDE_OLA_MESSAGING_DESCRIPTORVISITOR_H_
