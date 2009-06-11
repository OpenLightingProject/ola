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
 *
 * This implements a DmxBuffer which uses copy-on-write and delayed init.
 *
 * A DmxBuffer can hold up to 512 bytes of channel information. The amount of
 * valid data is returned by calling Size().
 */

#include <string.h>
#include <algorithm>
#include <vector>
#include <lla/BaseTypes.h>
#include <lla/DmxBuffer.h>
#include <lla/Logging.h>
#include <lla/StringUtils.h>

namespace lla {

using std::min;
using std::max;
using std::vector;

DmxBuffer::DmxBuffer():
  m_ref_count(NULL),
  m_copy_on_write(false),
  m_data(NULL),
  m_length(0) {
}


/*
 * Copy constructor. We just copy the underlying pointers and mark COW as
 * true if the other buffer has data.
 */
DmxBuffer::DmxBuffer(const DmxBuffer &other):
  m_ref_count(NULL),
  m_copy_on_write(false),
  m_data(NULL),
  m_length(0) {

  if (other.m_data && other.m_ref_count) {
    CopyFromOther(other);
  }
}


/*
 * Create a new buffer from data
 */
DmxBuffer::DmxBuffer(const uint8_t *data, unsigned int length):
  m_ref_count(0),
  m_copy_on_write(false),
  m_data(NULL),
  m_length(0) {
  Set(data, length);
}


/*
 * Create a new buffer from a string
 */
DmxBuffer::DmxBuffer(const string &data):
  m_ref_count(0),
  m_copy_on_write(false),
  m_data(NULL),
  m_length(0) {
    Set(data);
}


/*
 * Cleanup
 */
DmxBuffer::~DmxBuffer() {
  CleanupMemory();
}


/*
 * Make this buffer equal to another one
 * @param other the other DmxBuffer
 */
DmxBuffer& DmxBuffer::operator=(const DmxBuffer &other) {
  if (this != &other) {
    CleanupMemory();
    if (other.m_data) {
      CopyFromOther(other);
    }
  }
  return *this;
}


/*
 * Check for equality.
 */
bool DmxBuffer::operator==(const DmxBuffer &other) const {
  return (m_length == other.m_length &&
          (m_data == other.m_data ||
           0 == memcmp(m_data, other.m_data, m_length)));
}


/*
 * HTP Merge from another DmxBuffer.
 * @param other the DmxBuffer to HTP merge into this one
 */
bool DmxBuffer::HTPMerge(const DmxBuffer &other) {
  if (!m_data) {
    if(!Init())
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


/*
 * Set the contents of this DmxBuffer
 * @post Size() == length
 */
bool DmxBuffer::Set(const uint8_t *data, unsigned int length) {
  if (!data)
    return false;

  if (m_copy_on_write)
    CleanupMemory();
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
 * @post Size() == data.length()
 */
bool DmxBuffer::Set(const string &data) {
  return Set((uint8_t*) data.data(), data.length());
}


/*
 * Sets the data in this buffer to be the same as the other one.
 * Used instead of a COW to optimise.
 * @post Size() == other.Size()
 */
bool DmxBuffer::Set(const DmxBuffer &other) {
  return Set(other.m_data, other.m_length);
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


/*
 * Set a Range of data to a single value
 * @param offset the starting channel
 * @param value the value to set the range to
 * @param length the length of the range to set
 */
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


/*
 * Set a range of data. Calling this on an uninitialized buffer will call
 * Blackout() first. Attempting to set data with an offset > Size() is an
 * error.
 * @param offset the starting channel
 * @param data a pointer to the new data
 * @param length the length of the data
 */
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


/*
 * Set a single channel. Calling this on an uninitialized buffer will call
 * Blackout() first. Trying to set a channel more than 1 channel past the end
 * of the valid data is an error.
 */
void DmxBuffer::SetChannel(unsigned int channel, uint8_t data) {
  if (channel >= DMX_UNIVERSE_SIZE)
    return;

  if (!m_data) {
    Blackout();
  }

  if (channel > m_length) {
    LLA_WARN << "attempting to set channel " << channel << "when length is " <<
      m_length;
    return;
  }

  DuplicateIfNeeded();
  m_data[channel] = data;
  m_length = max(channel+1, m_length);
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
 * Returns the value of a channel. This returns 0 if the buffer wasn't
 * initialized or the channel was out-of-bounds.
 */
uint8_t DmxBuffer::Get(unsigned int channel) const {
  if (m_data && channel < m_length)
    return m_data[channel];
  else
    return 0;
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
 * Set the buffer to all zeros.
 * @post Size() == DMX_UNIVERSE_SIZE
 */
bool DmxBuffer::Blackout() {
  if (m_copy_on_write)
    CleanupMemory();
  if (!m_data)
    if (!Init())
      return false;
  bzero(m_data, DMX_UNIVERSE_SIZE);
  m_length = DMX_UNIVERSE_SIZE;
  return true;
}


/*
 * Reset the bufer to hold no data.
 * @post Size() == 0
 */
void DmxBuffer::Reset() {
  if (m_data)
    m_length = 0;
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
  try {
    m_ref_count = new unsigned int;
  } catch (std::bad_alloc& ex) {
    delete[] m_data;
    return false;
  }
  m_length = 0;
  *m_ref_count = 1;
  return true;
}


/*
 * Called before making a change, this duplicates the data if required.
 */
bool DmxBuffer::DuplicateIfNeeded() {
  if (m_copy_on_write && *m_ref_count == 1)
    m_copy_on_write = false;

  if (m_copy_on_write && *m_ref_count > 1) {
    unsigned int *old_ref_count = m_ref_count;
    uint8_t *original_data = m_data;
    unsigned int length = m_length;
    m_copy_on_write = false;
    Init();
    Set(original_data, length);
    (*old_ref_count)--;
  }
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

} // lla
