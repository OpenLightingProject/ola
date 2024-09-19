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
 * VariableFieldSizeCalculator.cpp
 * Copyright (C) 2011 Simon Newton
 */


#include <ola/messaging/Descriptor.h>
#include <vector>
#include "common/rdm/VariableFieldSizeCalculator.h"

namespace ola {
namespace rdm {

using ola::messaging::FieldDescriptorGroup;
using ola::messaging::StringFieldDescriptor;


/**
 * Figure out the size of a variable-length field in a descriptor. The variable
 * field may be a string or a repeated group of fields. Multiple-variable
 * length fields are not supported as this doesn't allow us to determine the
 * boundary of the individual fields within a message.
 *
 * This method is *not* re-entrant.
 * @param data_size the size in bytes of the data in this message
 * @param descriptor The descriptor to use to build the Message
 * @param variable_field_size a pointer to a int which is set to the length of
 * the variable field within this message.
 * @returns A enum which indicates if one (or more) variable length fields
 * exist, and if only one exists, what type it is.
 */
VariableFieldSizeCalculator::calculator_state
    VariableFieldSizeCalculator::CalculateFieldSize(
        unsigned int data_size,
        const class ola::messaging::Descriptor *descriptor,
        unsigned int *variable_field_size) {
  m_fixed_size_sum = 0;
  m_variable_string_fields.clear();
  m_variable_group_fields.clear();

  // split out the fields into fixed and variable length
  for (unsigned int i = 0; i < descriptor->FieldCount(); ++i)
    descriptor->GetField(i)->Accept(this);

  if (data_size < m_fixed_size_sum)
    return TOO_SMALL;

  unsigned int variable_string_field_count = m_variable_string_fields.size();
  unsigned int variable_group_field_count = m_variable_group_fields.size();

  if (variable_string_field_count + variable_group_field_count > 1)
    return MULTIPLE_VARIABLE_FIELDS;

  if (variable_string_field_count + variable_group_field_count == 0)
    return data_size > m_fixed_size_sum ? TOO_LARGE : FIXED_SIZE;

  // we know there is only one, now we need to work out the number of
  // repetitions or length if it's a string
  unsigned int bytes_remaining = data_size - m_fixed_size_sum;
  if (variable_string_field_count) {
    // variable string
    const StringFieldDescriptor *string_descriptor =
      m_variable_string_fields[0];

    if (bytes_remaining < string_descriptor->MinSize())
      return TOO_SMALL;
    if (bytes_remaining > string_descriptor->MaxSize())
      return TOO_LARGE;
    *variable_field_size = bytes_remaining;
    return VARIABLE_STRING;
  } else {
    // variable group
    const FieldDescriptorGroup *group_descriptor = m_variable_group_fields[0];
    if (!group_descriptor->FixedBlockSize())
      return NESTED_VARIABLE_GROUPS;

    unsigned int block_size = group_descriptor->BlockSize();
    if (group_descriptor->LimitedSize() &&
        bytes_remaining > block_size * group_descriptor->MaxBlocks())
      return TOO_LARGE;

    if (bytes_remaining % block_size)
      return MISMATCHED_SIZE;

    unsigned int repeat_count = bytes_remaining / block_size;
    if (repeat_count < group_descriptor->MinBlocks())
      return TOO_SMALL;

    if (group_descriptor->MaxBlocks() !=
          FieldDescriptorGroup::UNLIMITED_BLOCKS &&
        repeat_count >
          static_cast<unsigned int>(group_descriptor->MaxBlocks()))
      return TOO_LARGE;

    *variable_field_size = repeat_count;
    return VARIABLE_GROUP;
  }
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::BoolFieldDescriptor *descriptor) {
  m_fixed_size_sum += descriptor->MaxSize();
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::IPV4FieldDescriptor *descriptor) {
  m_fixed_size_sum += descriptor->MaxSize();
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::IPV6FieldDescriptor *descriptor) {
  m_fixed_size_sum += descriptor->MaxSize();
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::MACFieldDescriptor *descriptor) {
  m_fixed_size_sum += descriptor->MaxSize();
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::UIDFieldDescriptor *descriptor) {
  m_fixed_size_sum += descriptor->MaxSize();
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::StringFieldDescriptor *descriptor) {
  if (descriptor->FixedSize())
    m_fixed_size_sum += descriptor->MaxSize();
  else
    m_variable_string_fields.push_back(descriptor);
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::UInt8FieldDescriptor *descriptor) {
  m_fixed_size_sum += descriptor->MaxSize();
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::UInt16FieldDescriptor *descriptor) {
  m_fixed_size_sum += descriptor->MaxSize();
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::UInt32FieldDescriptor *descriptor) {
  m_fixed_size_sum += descriptor->MaxSize();
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::UInt64FieldDescriptor *descriptor) {
  m_fixed_size_sum += descriptor->MaxSize();
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::Int8FieldDescriptor *descriptor) {
  m_fixed_size_sum += descriptor->MaxSize();
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::Int16FieldDescriptor *descriptor) {
  m_fixed_size_sum += descriptor->MaxSize();
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::Int32FieldDescriptor *descriptor) {
  m_fixed_size_sum += descriptor->MaxSize();
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::Int64FieldDescriptor *descriptor) {
  m_fixed_size_sum += descriptor->MaxSize();
}


void VariableFieldSizeCalculator::Visit(
    const ola::messaging::FieldDescriptorGroup *descriptor) {
  if (descriptor->FixedSize())
    m_fixed_size_sum += descriptor->MaxSize();
  else
    m_variable_group_fields.push_back(descriptor);
}
}  // namespace rdm
}  // namespace ola
