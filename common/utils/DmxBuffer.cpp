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
 * DmxBuffer.cpp
 * The DmxBuffer class
 * Copyright (C) 2005-2009 Simon Newton
 *
 * This implements a DmxBuffer which uses copy-on-write and delayed init.
 *
 * A DmxBuffer can hold up to 512 bytes of channel information. The amount of
 * valid data is returned by calling Size().
 */

#include <string.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"

namespace ola {

using std::min;
using std::max;
using std::vector;

DmxBuffer::DmxBuffer()
    : m_ref_count(NULL),
      m_copy_on_write(false),
      m_data(NULL),
      m_length(0) {
}


DmxBuffer::DmxBuffer(const DmxBuffer &other)
    : m_ref_count(NULL),
      m_copy_on_write(false),
      m_data(NULL),
      m_length(0) {

  if (other.m_data && other.m_ref_count) {
    CopyFromOther(other);
  }
}


DmxBuffer::DmxBuffer(const uint8_t *data, unsigned int length)
    : m_ref_count(0),
      m_copy_on_write(false),
      m_data(NULL),
      m_length(0) {
  Set(data, length);
}


DmxBuffer::DmxBuffer(const string &data)
    : m_ref_count(0),
      m_copy_on_write(false),
      m_data(NULL),
      m_length(0) {
    Set(data);
}


DmxBuffer::~DmxBuffer() {
  CleanupMemory();
}


DmxBuffer& DmxBuffer::operator=(const DmxBuffer &other) {
  if (this != &other) {
    CleanupMemory();
    if (other.m_data) {
      CopyFromOther(other);
    }
  }
  return *this;
}


bool DmxBuffer::operator==(const DmxBuffer &other) const {
  return (m_length == other.m_length &&
          (m_data == other.m_data ||
           0 == memcmp(m_data, other.m_data, m_length)));
}


bool DmxBuffer::operator!=(const DmxBuffer &other) const {
  return !(*this == other);
}


bool DmxBuffer::HTPMerge(const DmxBuffer &other) {
  if (!m_data) {
    if (!Init())
      return false;
  }
  DuplicateIfNeeded();

  unsigned int other_length = min((unsigned int) DMX_UNIVERSE_SIZE,
                                  other.m_length);
  unsigned int merge_length = min(m_length, other.m_length);

  for (unsigned int i = 0; i < merge_length; i++) {
    m_data[i] = max(m_data[i], other.m_data[i]);
  }

  if (other_length > m_length) {
    memcpy(m_data + merge_length, other.m_data + merge_length,
           other_length - merge_length);
    m_length = other_length;
  }
  return true;
}


bool DmxBuffer::Set(const uint8_t *data, unsigned int length) {
  if (!data)
    return false;

  if (m_copy_on_write)
    CleanupMemory();
  if (!m_data) {
    if (!Init())
      return false;
  }
  m_length = min(length, (unsigned int) DMX_UNIVERSE_SIZE);
  memcpy(m_data, data, m_length);
  return true;
}


bool DmxBuffer::Set(const string &data) {
  return Set(reinterpret_cast<const uint8_t*>(data.data()), data.length());
}


bool DmxBuffer::Set(const DmxBuffer &other) {
  return Set(other.m_data, other.m_length);
}


bool DmxBuffer::SetFromString(const string &input) {
  unsigned int i = 0;
  vector<string> dmx_values;
  vector<string>::const_iterator iter;

  if (m_copy_on_write)
    CleanupMemory();
  if (!m_data)
    if (!Init())
      return false;

  if (input.empty()) {
    m_length = 0;
    return true;
  }
  StringSplit(input, dmx_values, ",");
  for (iter = dmx_values.begin();
      iter != dmx_values.end() && i < DMX_UNIVERSE_SIZE; ++iter, ++i) {
    m_data[i] = atoi(iter->data());
  }
  m_length = i;
  return true;
}


bool DmxBuffer::SetRangeToValue(unsigned int offset,
                                uint8_t value,
                                unsigned int length) {
  if (offset >= DMX_UNIVERSE_SIZE)
    return false;

  if (!m_data) {
    Blackout();
  }

  if (offset > m_length)
    return false;

  DuplicateIfNeeded();

  unsigned int copy_length = min(length, DMX_UNIVERSE_SIZE - offset);
  memset(m_data + offset, value, copy_length);
  m_length = max(m_length, offset + copy_length);
  return true;
}


bool DmxBuffer::SetRange(unsigned int offset,
                         const uint8_t *data,
                         unsigned int length) {
  if (!data || offset >= DMX_UNIVERSE_SIZE)
    return false;

  if (!m_data) {
    Blackout();
  }

  if (offset > m_length)
    return false;

  DuplicateIfNeeded();

  unsigned int copy_length = min(length, DMX_UNIVERSE_SIZE - offset);
  memcpy(m_data + offset, data, copy_length);
  m_length = max(m_length, offset + copy_length);
  return true;
}


void DmxBuffer::SetChannel(unsigned int channel, uint8_t data) {
  if (channel >= DMX_UNIVERSE_SIZE)
    return;

  if (!m_data) {
    Blackout();
  }

  if (channel > m_length) {
    OLA_WARN << "attempting to set channel " << channel << "when length is " <<
      m_length;
    return;
  }

  DuplicateIfNeeded();
  m_data[channel] = data;
  m_length = max(channel+1, m_length);
}


void DmxBuffer::Get(uint8_t *data, unsigned int *length) const {
  if (m_data) {
    *length = min(*length, m_length);
    memcpy(data, m_data, *length);
  } else {
    *length = 0;
  }
}


void DmxBuffer::GetRange(unsigned int slot, uint8_t *data,
                         unsigned int *length) const {
  if (m_data) {
    *length = min(*length, m_length - slot);
    memcpy(data, m_data + slot, *length);
  } else {
    *length = 0;
  }
}


uint8_t DmxBuffer::Get(unsigned int channel) const {
  if (m_data && channel < m_length)
    return m_data[channel];
  else
    return 0;
}


string DmxBuffer::Get() const {
  string data;
  data.append(reinterpret_cast<char*>(m_data), m_length);
  return data;
}


/*
 * Set the buffer to all zeros.
 * @post Size() == DMX_UNIVERSE_SIZE
 */
bool DmxBuffer::Blackout() {
  if (m_copy_on_write)
    CleanupMemory();
  if (!m_data)
    if (!Init())
      return false;
  memset(m_data, DMX_MIN_CHANNEL_VALUE, DMX_UNIVERSE_SIZE);
  m_length = DMX_UNIVERSE_SIZE;
  return true;
}


void DmxBuffer::Reset() {
  if (m_data)
    m_length = 0;
}


string DmxBuffer::ToString() const {
  if (!m_data)
    return "";

  std::stringstream str;
  for (unsigned int i = 0; i < Size(); i++) {
    if (i)
      str << ",";
    str << static_cast<int>(m_data[i]);
  }
  return str.str();
}


/*
 * Allocate memory
 * @return true on success, otherwise raises an exception
 */
bool DmxBuffer::Init() {
  m_data = new uint8_t[DMX_UNIVERSE_SIZE];
  m_ref_count = new unsigned int;
  m_length = 0;
  *m_ref_count = 1;
  return true;
}


/*
 * Called before making a change, this duplicates the data if required.
 * @return true on Duplication, and false it duplication was not needed
 */
bool DmxBuffer::DuplicateIfNeeded() {
  if (m_copy_on_write && *m_ref_count == 1)
    m_copy_on_write = false;

  if (m_copy_on_write && *m_ref_count > 1) {
    unsigned int *old_ref_count = m_ref_count;
    uint8_t *original_data = m_data;
    unsigned int length = m_length;
    m_copy_on_write = false;
    if (Init()) {
      Set(original_data, length);
      (*old_ref_count)--;
      return true;
    }
    return false;
  }
  return true;
}


/*
 * Setup this buffer to point to the data of the other buffer
 * @param other the source buffer
 * @pre other.m_data and other.m_ref_count are not NULL
 */
void DmxBuffer::CopyFromOther(const DmxBuffer &other) {
  m_copy_on_write = true;
  other.m_copy_on_write = true;
  m_ref_count = other.m_ref_count;
  (*m_ref_count)++;
  m_data = other.m_data;
  m_length = other.m_length;
}


/*
 * Decrement the ref count by one and free the memory if required
 */
void DmxBuffer::CleanupMemory() {
  if (m_ref_count && m_data) {
    (*m_ref_count)--;
    if (!*m_ref_count) {
      delete[] m_data;
      delete m_ref_count;
    }
    m_data = NULL;
    m_ref_count = NULL;
    m_length = 0;
  }
}

/*
 * Stream operator to allow DmxBuffer to be output to stdout
 *
 * @example DmxBuffer dmx_buffer();
 *          cout << dmx_buffer << endl; //Show channel values of
 *                                      //dmx_buffer
 */
std::ostream& operator<<(std::ostream &out, const DmxBuffer &data) {
  return out << data.ToString();
}
}  // namespace  ola
