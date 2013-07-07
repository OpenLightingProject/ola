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
 * DmxBuffer.h
 * Interface for the DmxBuffer
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef INCLUDE_OLA_DMXBUFFER_H_
#define INCLUDE_OLA_DMXBUFFER_H_

#include <stdint.h>
#include <iostream>
#include <string>

namespace ola {

using std::string;

/**
 * @class DmxBuffer ola/DmxBuffer.h
 * @brief Used to hold a single DMX universe of data
 *
 * DmxBuffer is used to hold a single universe of dmx data. This class includes
 * functions to translate to/from strings, and manipulate channels in the 
 * buffer
 *
 * @note DmxBuffer uses a copy-on-write (COW) optimization, more info can be 
 * found here: http://en.wikipedia.org/wiki/Copy-on-write
 * 
 * @note This class is <b>NOT<\b> thread safe
 */
class DmxBuffer {
  public:
    /**
     * Constructor
     */
    DmxBuffer();

    /*
     * Copy constructor. We just copy the underlying pointers and mark 
     * m_copy_on_write as true if the other buffer has data.
     * @param other The other DmxBuffer to copy from
     */
    DmxBuffer(const DmxBuffer &other);
    
    /**
     * Create a new buffer from raw data
     * @param data is a pointer to an array of data used to populate DmxBufer
     * @param length is the length of data in array data
     */
    DmxBuffer(const uint8_t *data, unsigned int length);
    
    /**
     * Create a new buffer from a string
     * @param data is a string of raw data values
     * 
     * @deprecated Use DmxBuffer(const uint8_t *data, unsigned int length)
     * instead
     */
    explicit DmxBuffer(const string &data);

    /**
     * Deconstructor calls CleanupMemory
     */
    ~DmxBuffer();

    /**
     * Assignmnent operator used to make this buffer equal to another buffer
     * @param other the other DmxBuffer to copy/link from
     */    
    DmxBuffer& operator=(const DmxBuffer &other);

    /**
     * Equality operator used to check if two DmxBuffers are equal
     * @param other is the other DmxBuffer to check against
     * @return true if equal, and false if not
     */
    bool operator==(const DmxBuffer &other) const;

    /**
     * Inequality operator used to check if two DmxBuffers are not equal
     * @param other is the other DmxBuffer to check against
     * @return true if not equal and false if the are equal
     */
    bool operator!=(const DmxBuffer &other) const;
    
    /**
     * Returns th size of the buffer
     *
     */
    unsigned int Size() const { return m_length; }

    bool HTPMerge(const DmxBuffer &other);
    bool Set(const uint8_t *data, unsigned int length);
    bool Set(const string &data);
    bool Set(const DmxBuffer &other);
    bool SetFromString(const string &data);
    bool SetRangeToValue(unsigned int offset, uint8_t data,
                         unsigned int length);
    bool SetRange(unsigned int offset, const uint8_t *data,
                  unsigned int length);
    void SetChannel(unsigned int channel, uint8_t data);
    void Get(uint8_t *data, unsigned int *length) const;
    void GetRange(unsigned int slot, uint8_t *data,
                  unsigned int *length) const;
    uint8_t Get(unsigned int channel) const;
    const uint8_t *GetRaw() const { return m_data; }
    string Get() const;
    bool Blackout();
    void Reset();
    string ToString() const;

  private:
    bool Init();
    bool DuplicateIfNeeded();
    void CopyFromOther(const DmxBuffer &other);
    void CleanupMemory();
    unsigned int *m_ref_count;
    mutable bool m_copy_on_write;
    uint8_t *m_data;
    unsigned int m_length;
};

std::ostream& operator<<(std::ostream &out, const DmxBuffer &data);
}  // namespace ola
#endif  // INCLUDE_OLA_DMXBUFFER_H_
