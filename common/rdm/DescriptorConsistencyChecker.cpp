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
 * DescriptorConsistencyChecker.cpp
 * Verify that we can determine the layout of a Descriptor.
 * Copyright (C) 2011 Simon Newton
 */

#include "common/rdm/DescriptorConsistencyChecker.h"

namespace ola {
namespace rdm {


bool DescriptorConsistencyChecker::CheckConsistency(
    const ola::messaging::Descriptor *descriptor) {
  m_variable_sized_field_count = 0;
  descriptor->Accept(this);
  return m_variable_sized_field_count <= 1;
}

void DescriptorConsistencyChecker::Visit(
    const ola::messaging::BoolFieldDescriptor*) {
}


void DescriptorConsistencyChecker::Visit(
    const ola::messaging::IPV4FieldDescriptor*) {
}


void DescriptorConsistencyChecker::Visit(
    const ola::messaging::IPV6FieldDescriptor*) {
}


void DescriptorConsistencyChecker::Visit(
    const ola::messaging::MACFieldDescriptor*) {
}


void DescriptorConsistencyChecker::Visit(
    const ola::messaging::UIDFieldDescriptor*) {
}


void DescriptorConsistencyChecker::Visit(
  const ola::messaging::StringFieldDescriptor *descriptor) {
    if (!descriptor->FixedSize()) {
      m_variable_sized_field_count++;
    }
}


void DescriptorConsistencyChecker::Visit(
    const ola::messaging::UInt8FieldDescriptor*) {
}


void DescriptorConsistencyChecker::Visit(
    const ola::messaging::UInt16FieldDescriptor*) {
}


void DescriptorConsistencyChecker::Visit(
    const ola::messaging::UInt32FieldDescriptor*) {
}


void DescriptorConsistencyChecker::Visit(
    const ola::messaging::UInt64FieldDescriptor*) {
}


void DescriptorConsistencyChecker::Visit(
    const ola::messaging::Int8FieldDescriptor*) {
}


void DescriptorConsistencyChecker::Visit(
    const ola::messaging::Int16FieldDescriptor*) {
}


void DescriptorConsistencyChecker::Visit(
    const ola::messaging::Int32FieldDescriptor*) {
}


void DescriptorConsistencyChecker::Visit(
    const ola::messaging::Int64FieldDescriptor*) {
}


void DescriptorConsistencyChecker::Visit(
    const ola::messaging::FieldDescriptorGroup *descriptor) {
  if (!descriptor->FixedSize())
    m_variable_sized_field_count++;
  // if the block size isn't fixed this descriptor isn't consistent.
  if (!descriptor->FixedBlockSize())
    m_variable_sized_field_count++;
}


void DescriptorConsistencyChecker::PostVisit(
    const ola::messaging::FieldDescriptorGroup*) {
}
}  // namespace rdm
}  // namespace ola
