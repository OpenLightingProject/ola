/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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

using std::vector;
using ola::messaging::FieldDescriptorInterface;
using ola::messaging::StringFieldDescriptor;
using ola::messaging::FieldDescriptorGroup;


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

    VariableFieldSizeCalculator() {}
    ~VariableFieldSizeCalculator() {}

    bool Descend() const { return false; }
    calculator_state CalculateFieldSize(
        unsigned int data_size,
        const class ola::messaging::Descriptor*,
        unsigned int *variable_field_repeat_count);

    void Visit(const ola::messaging::BoolFieldDescriptor*);
    void Visit(const ola::messaging::StringFieldDescriptor*);
    void Visit(const ola::messaging::UInt8FieldDescriptor*);
    void Visit(const ola::messaging::UInt16FieldDescriptor*);
    void Visit(const ola::messaging::UInt32FieldDescriptor*);
    void Visit(const ola::messaging::Int8FieldDescriptor*);
    void Visit(const ola::messaging::Int16FieldDescriptor*);
    void Visit(const ola::messaging::Int32FieldDescriptor*);
    void Visit(const ola::messaging::FieldDescriptorGroup*);
    void PostVisit(const ola::messaging::FieldDescriptorGroup*) {}

  private:
    unsigned int m_fixed_size_sum;
    vector<const StringFieldDescriptor*> m_variable_string_fields;
    vector<const FieldDescriptorGroup*> m_variable_group_fields;

    unsigned int DetermineGroupSize(const
        ola::messaging::FieldDescriptorGroup*);
};
}  // rdm
}  // ola
#endif  // COMMON_RDM_VARIABLEFIELDSIZECALCULATOR_H_
