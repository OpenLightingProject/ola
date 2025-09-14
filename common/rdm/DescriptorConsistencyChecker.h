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
 * DescriptorConsistencyChecker.h
 * Verify that we can determine the layout of a Descriptor.
 *
 * In order for the binary unpacker to work, we need to be able to determine
 * the size and offset of every field within a descriptor, without consdering
 * the data itself. This means the following are unsupported:
 *
 *  - nested non-fixed sized groups
 *  - multiple variable-sized fields e.g multiple strings
 *  - variable-sized fields within groups
 *
 * Copyright (C) 2011 Simon Newton
 */

#ifndef COMMON_RDM_DESCRIPTORCONSISTENCYCHECKER_H_
#define COMMON_RDM_DESCRIPTORCONSISTENCYCHECKER_H_

#include <ola/messaging/DescriptorVisitor.h>
#include <ola/messaging/Descriptor.h>

namespace ola {
namespace rdm {


/**
 * Check that a RDM Descriptor is consistent.
 */
class DescriptorConsistencyChecker
    : public ola::messaging::FieldDescriptorVisitor {
 public:
    DescriptorConsistencyChecker()
        : m_variable_sized_field_count(0) {
    }

    bool Descend() const { return false; }
    bool CheckConsistency(const ola::messaging::Descriptor *descriptor);

    void Visit(const ola::messaging::BoolFieldDescriptor*);
    void Visit(const ola::messaging::IPV4FieldDescriptor*);
    void Visit(const ola::messaging::IPV6FieldDescriptor*);
    void Visit(const ola::messaging::MACFieldDescriptor*);
    void Visit(const ola::messaging::UIDFieldDescriptor*);
    void Visit(const ola::messaging::StringFieldDescriptor*);
    void Visit(const ola::messaging::UInt8FieldDescriptor*);
    void Visit(const ola::messaging::UInt16FieldDescriptor*);
    void Visit(const ola::messaging::UInt32FieldDescriptor*);
    void Visit(const ola::messaging::UInt64FieldDescriptor*);
    void Visit(const ola::messaging::Int8FieldDescriptor*);
    void Visit(const ola::messaging::Int16FieldDescriptor*);
    void Visit(const ola::messaging::Int32FieldDescriptor*);
    void Visit(const ola::messaging::Int64FieldDescriptor*);
    void Visit(const ola::messaging::FieldDescriptorGroup*);
    void PostVisit(const ola::messaging::FieldDescriptorGroup*);

 private:
    unsigned int m_variable_sized_field_count;
};
}  // namespace rdm
}  // namespace ola
#endif  // COMMON_RDM_DESCRIPTORCONSISTENCYCHECKER_H_
