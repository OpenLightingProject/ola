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
 * GroupSizeCalculator.h
 * Calculate the number of items in a group, given a fixed number of tokens.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef COMMON_RDM_GROUPSIZECALCULATOR_H_
#define COMMON_RDM_GROUPSIZECALCULATOR_H_

#include <ola/messaging/DescriptorVisitor.h>
#include <stack>
#include <vector>

namespace ola {

namespace messaging {
class Descriptor;
}

namespace rdm {


/**
 * This calculates the required nmuber of tokens for a group which contains no
 * variable-sized groups.
 */
class StaticGroupTokenCalculator
    : public ola::messaging::FieldDescriptorVisitor {
 public:
    StaticGroupTokenCalculator()
        : m_variable_sized_group_encountered(false) {
    }
    ~StaticGroupTokenCalculator() {}

    bool Descend() const { return true; }
    bool CalculateTokensRequired(
        const class ola::messaging::FieldDescriptorGroup*,
        unsigned int *token_count);

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
    std::stack<unsigned int> m_token_count;
    bool m_variable_sized_group_encountered;
};


/**
 * Calculate the number of repeats of a group required, given a certain number
 * of tokens.
 */
class GroupSizeCalculator: public ola::messaging::FieldDescriptorVisitor {
 public:
    typedef enum {
      INSUFFICIENT_TOKENS,
      EXTRA_TOKENS,
      NO_VARIABLE_GROUPS,
      SINGLE_VARIABLE_GROUP,
      MULTIPLE_VARIABLE_GROUPS,
      NESTED_VARIABLE_GROUPS,
      MISMATCHED_TOKENS,
    } calculator_state;

    GroupSizeCalculator() {}
    ~GroupSizeCalculator() {}

    bool Descend() const { return false; }
    calculator_state CalculateGroupSize(
        unsigned int token_count,
        const class ola::messaging::Descriptor *descriptor,
        unsigned int *group_repeat_count);

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
    std::vector<const ola::messaging::FieldDescriptorGroup*> m_groups;
    std::vector<const ola::messaging::FieldDescriptorInterface*> m_non_groups;
    StaticGroupTokenCalculator m_simple_calculator;

    unsigned int DetermineGroupSize(const
        ola::messaging::FieldDescriptorGroup*);
};
}  // namespace rdm
}  // namespace ola
#endif  // COMMON_RDM_GROUPSIZECALCULATOR_H_
