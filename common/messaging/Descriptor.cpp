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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Descriptor.cpp
 * Holds the metadata (schema) for a Message.
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/messaging/Descriptor.h>
#include <vector>

namespace ola {
namespace messaging {

using std::vector;

const int16_t FieldDescriptorGroup::UNLIMITED_BLOCKS = -1;

FieldDescriptorGroup::~FieldDescriptorGroup() {
  vector<const FieldDescriptor*>::const_iterator iter = m_fields.begin();
  for (; iter != m_fields.end(); ++iter)
    delete *iter;
}


bool FieldDescriptorGroup::LimitedSize() const {
  if (m_max_blocks == UNLIMITED_BLOCKS)
    return false;

  PopulateIfRequired();
  return m_limited_size;
}


bool FieldDescriptorGroup::FixedBlockSize() const {
  PopulateIfRequired();
  return m_fixed_size;
}


unsigned int FieldDescriptorGroup::BlockSize() const {
  PopulateIfRequired();
  return m_block_size;
}


unsigned int FieldDescriptorGroup::MaxBlockSize() const {
  PopulateIfRequired();
  return m_max_block_size;
}


unsigned int FieldDescriptorGroup::MaxSize() const {
  if (!LimitedSize())
    return 0;
  return MaxBlockSize() * m_max_blocks;
}


void FieldDescriptorGroup::Accept(FieldDescriptorVisitor &visitor) const {
  visitor.Visit(this);
  vector<const FieldDescriptor*>::const_iterator iter = m_fields.begin();
  if (visitor.Descend()) {
    for (; iter != m_fields.end(); ++iter)
      (*iter)->Accept(visitor);
  }
  visitor.PostVisit(this);
}


/**
 * We cache all the information that requires iterating over the fields
 * This method populates the cache if required.
 */
void FieldDescriptorGroup::PopulateIfRequired() const {
  if (m_populated)
    return;
  unsigned int size = 0;
  vector<const FieldDescriptor*>::const_iterator iter = m_fields.begin();
  for (; iter != m_fields.end(); ++iter) {
    if (!(*iter)->LimitedSize())
      m_limited_size = false;
    if (!(*iter)->FixedSize())
      m_fixed_size = false;
    size += (*iter)->MaxSize();
  }
  m_populated = true;
  m_block_size = m_fixed_size ? size : 0;
  m_max_block_size = m_limited_size ? size : 0;
}


void Descriptor::Accept(FieldDescriptorVisitor &visitor) const {
  vector<const FieldDescriptor*>::const_iterator iter = m_fields.begin();
  for (; iter != m_fields.end(); ++iter)
    (*iter)->Accept(visitor);
}
}  // messaging
}  // ola
