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
 * GroupSizeCalculator.cpp
 * Copyright (C) 2011 Simon Newton
 */


#include <ola/messaging/Descriptor.h>
#include <vector>
#include "common/rdm/GroupSizeCalculator.h"

namespace ola {
namespace rdm {

using ola::messaging::FieldDescriptor;
using ola::messaging::FieldDescriptorGroup;


/**
 * Figure out the number of group repeatitions required.
 *
 * This method is *not* re-entrant.
 * @param descriptor The descriptor to use to build the Message
 * @returns A Message object, or NULL if the inputs failed.
 */
GroupSizeCalculator::calculator_state GroupSizeCalculator::CalculateGroupSize(
    unsigned int token_count,
    const ola::messaging::Descriptor *descriptor,
    unsigned int *group_repeat_count) {

  m_groups.clear();
  m_non_groups.clear();

  // split out the fields into singular fields and groups
  for (unsigned int i = 0; i < descriptor->FieldCount(); ++i) {
    descriptor->GetField(i)->Accept(*this);
  }

  unsigned int required_length = m_non_groups.size();

  if (required_length > token_count)
    return INSUFFICIENT_INPUTS;

  // this takes care of the easy case where there are no groups
  if (!m_groups.size())
    return required_length == token_count ? NO_VARIABLE_GROUPS : EXTRA_INPUTS;

  // check all groups, looking for multiple non-fixed sized groups
  unsigned int variable_group_count = 0;
  const FieldDescriptorGroup *variable_group = NULL;
  vector<const FieldDescriptorGroup*>::const_iterator iter = m_groups.begin();
  for (; iter != m_groups.end(); ++iter) {
    if (!(*iter)->FixedSize()) {
      variable_group = *iter;
      variable_group_count++;
    }
  }

  if (variable_group_count > 1)
    return MULTIPLE_VARIABLE_GROUPS;

  // work out the number of inputs for all fixed sized groups
  iter = m_groups.begin();
  for (; iter != m_groups.end(); ++iter) {
    if (*iter == variable_group)
      continue;
    // recursively get group size
    unsigned int group_size;
    if (!m_simple_calculator.CalculateTokensRequired(*iter, &group_size))
      return NESTED_VARIABLE_GROUPS;
    required_length += (*iter)->Size() * group_size;
  }

  if (required_length > token_count)
    return INSUFFICIENT_INPUTS;

  if (variable_group_count == 0)
    return required_length == token_count ? NO_VARIABLE_GROUPS : EXTRA_INPUTS;

  // now we have a single variable sized group and a non-0 number of inputs
  // remaining.
  unsigned int group_size;
  if (!m_simple_calculator.CalculateTokensRequired(variable_group,
                                                   &group_size))
    return NESTED_VARIABLE_GROUPS;

  unsigned int remaining_inputs = token_count - required_length;
  if (remaining_inputs % group_size)
    return MISMATCHED_INPUTS;

  *group_repeat_count = remaining_inputs / group_size;
  return SINGLE_VARIABLE_GROUP;
}


void GroupSizeCalculator::Visit(
    const ola::messaging::BoolFieldDescriptor *descriptor) {
  m_non_groups.push_back(descriptor);
}


void GroupSizeCalculator::Visit(
    const ola::messaging::StringFieldDescriptor *descriptor) {
  m_non_groups.push_back(descriptor);
}


void GroupSizeCalculator::Visit(
    const ola::messaging::UInt8FieldDescriptor *descriptor) {
  m_non_groups.push_back(descriptor);
}


void GroupSizeCalculator::Visit(
    const ola::messaging::UInt16FieldDescriptor *descriptor) {
  m_non_groups.push_back(descriptor);
}


void GroupSizeCalculator::Visit(
    const ola::messaging::UInt32FieldDescriptor *descriptor) {
  m_non_groups.push_back(descriptor);
}


void GroupSizeCalculator::Visit(
    const ola::messaging::Int8FieldDescriptor *descriptor) {
  m_non_groups.push_back(descriptor);
}


void GroupSizeCalculator::Visit(
    const ola::messaging::Int16FieldDescriptor *descriptor) {
  m_non_groups.push_back(descriptor);
}


void GroupSizeCalculator::Visit(
    const ola::messaging::Int32FieldDescriptor *descriptor) {
  m_non_groups.push_back(descriptor);
}


void GroupSizeCalculator::Visit(
    const ola::messaging::FieldDescriptorGroup *descriptor) {
  m_groups.push_back(descriptor);
}


void GroupSizeCalculator::PostVisit(
    const ola::messaging::FieldDescriptorGroup *descriptor) {
  (void) descriptor;
}


// StaticGroupTokenCalculator
//-----------------------------------------------------------------------------

/**
 * For a group of fields, figure out the number of inputs required to build a
 * single instance of the group. This assumes that the group does not contain
 * any variable-sized groups but it may contain fixed sized nested groups.
 * @param descriptor the group descriptor
 * @param group_input_size the number of inputs required to build a single
 * instance of this group.
 * @return true if we could calculate the inputs required, false if this group
 * was of a variable size.
 */
bool StaticGroupTokenCalculator::CalculateTokensRequired(
    const class ola::messaging::FieldDescriptorGroup *descriptor,
    unsigned int *token_count) {

  // reset the stack
  while (!m_token_count.empty())
    m_token_count.pop();
  m_token_count.push(0);
  m_variable_sized_group_encountered = false;

  for (unsigned int i = 0; i < descriptor->FieldCount(); ++i)
    descriptor->GetField(i)->Accept(*this);

  if (m_variable_sized_group_encountered)
    return false;

  *token_count = m_token_count.top();
  m_token_count.pop();
  return true;
}

void StaticGroupTokenCalculator::Visit(
    const ola::messaging::BoolFieldDescriptor *descriptor) {
  m_token_count.top()++;
  (void) descriptor;
}


void StaticGroupTokenCalculator::Visit(
    const ola::messaging::StringFieldDescriptor *descriptor) {
  m_token_count.top()++;
  (void) descriptor;
}


void StaticGroupTokenCalculator::Visit(
    const ola::messaging::UInt8FieldDescriptor *descriptor) {
  m_token_count.top()++;
  (void) descriptor;
}


void StaticGroupTokenCalculator::Visit(
    const ola::messaging::UInt16FieldDescriptor *descriptor) {
  m_token_count.top()++;
  (void) descriptor;
}


void StaticGroupTokenCalculator::Visit(
    const ola::messaging::UInt32FieldDescriptor *descriptor) {
  m_token_count.top()++;
  (void) descriptor;
}


void StaticGroupTokenCalculator::Visit(
    const ola::messaging::Int8FieldDescriptor *descriptor) {
  m_token_count.top()++;
  (void) descriptor;
}


void StaticGroupTokenCalculator::Visit(
    const ola::messaging::Int16FieldDescriptor *descriptor) {
  m_token_count.top()++;
  (void) descriptor;
}


void StaticGroupTokenCalculator::Visit(
    const ola::messaging::Int32FieldDescriptor *descriptor) {
  m_token_count.top()++;
  (void) descriptor;
}


void StaticGroupTokenCalculator::Visit(
    const ola::messaging::FieldDescriptorGroup *descriptor) {
  m_token_count.push(0);
  if (!descriptor->FixedSize())
    m_variable_sized_group_encountered = true;
}


void StaticGroupTokenCalculator::PostVisit(
    const ola::messaging::FieldDescriptorGroup *descriptor) {
  unsigned int group_length = m_token_count.top();
  m_token_count.pop();
  m_token_count.top() += group_length * descriptor->Size();
}
}  // rdm
}  // ola
