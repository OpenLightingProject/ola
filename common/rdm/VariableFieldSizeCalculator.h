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
 * VariableFieldSizeCalculator.h
 * Calculate the number of items in a group, given a fixed number of tokens.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef COMMON_RDM_VARIABLEFIELDSIZECALCULATOR_H_
#define COMMON_RDM_VARIABLEFIELDSIZECALCULATOR_H_

#include <ola/messaging/DescriptorVisitor.h>
#include <stack>
#include <vector>

namespace ola {

namespace messaging {
class Descriptor;
}

namespace rdm {


/**
 * Calculate the size of a variable field when unpacking a Message from a raw
 * data stream.
 */
class VariableFieldSizeCalculator
    : public ola::messaging::FieldDescriptorVisitor {
 public:
    typedef enum {
      TOO_SMALL,
      TOO_LARGE,
      FIXED_SIZE,
      VARIABLE_STRING,
      VARIABLE_GROUP,
      MULTIPLE_VARIABLE_FIELDS,
      NESTED_VARIABLE_GROUPS,
      MISMATCHED_SIZE,
    } calculator_state;

    VariableFieldSizeCalculator() : m_fixed_size_sum(0) {}
    ~VariableFieldSizeCalculator() {}

    bool Descend() const { return false; }
    calculator_state CalculateFieldSize(
        unsigned int data_size,
        const class ola::messaging::Descriptor*,
        unsigned int *variable_field_repeat_count);

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
    void PostVisit(const ola::messaging::FieldDescriptorGroup*) {}

 private:
    unsigned int m_fixed_size_sum;
    std::vector<const ola::messaging::StringFieldDescriptor*>
        m_variable_string_fields;
    std::vector<const ola::messaging::FieldDescriptorGroup*>
        m_variable_group_fields;

    unsigned int DetermineGroupSize(const
        ola::messaging::FieldDescriptorGroup*);
};
}  // namespace rdm
}  // namespace ola
#endif  // COMMON_RDM_VARIABLEFIELDSIZECALCULATOR_H_
