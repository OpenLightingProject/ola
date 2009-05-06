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
 * DmxBuffer.cpp
 * The DmxBuffer class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string.h>
#include <algorithm>
#include <vector>
#include <lla/BaseTypes.h>
#include <lla/DmxBuffer.h>
#include <lla/StringUtils.h>

namespace lla {

using std::min;
using std::max;
using std::vector;

DmxBuffer::DmxBuffer():
  m_data(NULL),
  m_length(0) {
}


/*
 * Copy
 */
DmxBuffer::DmxBuffer(const DmxBuffer &other):
  m_data(NULL),
  m_length(0) {
  Set(other.m_data, other.m_length);
}


/*
 * Create a new buffer from data
 */
DmxBuffer::DmxBuffer(const uint8_t *data, unsigned int length):
  m_data(NULL),
  m_length(0) {
  Set(data, length);
}


/*
 * Cleanup
 */
DmxBuffer::~DmxBuffer() {
  delete[] m_data;
}


/*
 * Assign
 */
DmxBuffer& DmxBuffer::operator=(const DmxBuffer &other) {
  if (this != &other) {
    if (other.m_data) {
      if (!m_data) {
        Init();
      }
      m_length = min(other.m_length, (unsigned int) DMX_UNIVERSE_SIZE);
      memcpy(m_data, other.m_data, m_length);
    } else {
      m_length = 0;
    }
  }
  return *this;
}


/*
 * Check for equality
 */
bool DmxBuffer::operator==(const DmxBuffer &other) const {
  return (m_length == other.m_length &&
          0 == memcmp(m_data, other.m_data, m_length));
}


/*
 * HTP Merge from another DmxBuffer
 */
bool DmxBuffer::HTPMerge(const DmxBuffer &other) {
  if (!m_data) {
    if(!Init())
      return false;
  }

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


/*
 * Set the contents of this DmxBuffer
 */
bool DmxBuffer::Set(const uint8_t *data, unsigned int length) {
  if (!m_data) {
    if(!Init())
      return false;
  }
  m_length = min(length, (unsigned int) DMX_UNIVERSE_SIZE);
  memcpy(m_data, data, m_length);
  return true;
}


/*
 * Set the contents of this DmxBuffer
 * @param data the string with the dmx data
 */
bool DmxBuffer::Set(const string &data) {
  return Set((uint8_t*) data.data(), data.length());
}


/*
 * Convert a ',' separated list into a dmx_t array. Invalid values are set to
 * 0. 0s can be dropped between the commas.
 * @param input the string to split
 */
bool DmxBuffer::SetFromString(const string &input) {
  unsigned int i = 0;
  vector<string> dmx_values;
  vector<string>::const_iterator iter;

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


/*
 * Get the contents of this buffer
 */
void DmxBuffer::Get(uint8_t *data, unsigned int &length) const {
  if (m_data) {
    length = min(length, m_length);
    memcpy(data, m_data, length);
  } else {
    length = 0;
  }
}


/*
 * Get the contents of the DmxBuffer as a string
 */
string DmxBuffer::Get() const {
  string data;
  data.append((char*) m_data, m_length);
  return data;
}


/*
 * Allocate memory
 */
bool DmxBuffer::Init() {
  try {
    m_data = new uint8_t[DMX_UNIVERSE_SIZE];
  } catch (std::bad_alloc& ex) {
    return false;
  }
  m_length = 0;
  return true;
}

} // lla
