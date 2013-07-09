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

/**
 * @file DmxBuffer.h
 * @brief A class used to hold a single universe of DMX data.
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
 * @brief Used to hold a single universe of DMX data.
 *
 * DmxBuffer is used to hold a single universe of DMX data. This class includes
 * functions to translate to/from strings, and manipulate channels in the
 * buffer.
 *
 * @note DmxBuffer uses a copy-on-write (COW) optimization, more info can be
 * found here: http://en.wikipedia.org/wiki/Copy-on-write
 *
 * @note This class is <b>NOT</b> thread safe.
 */
class DmxBuffer {
  public:
    /**
     * Constructor
     * This initializes and empty DmxBuffer, Size() == 0
     */
    DmxBuffer();

    /**
     * @brief Copy constructor.
     * We just copy the underlying pointers and mark
     * m_copy_on_write as true if the other buffer has data.
     * @param other The other DmxBuffer to copy from
     */
    DmxBuffer(const DmxBuffer &other);

    /**
     * @brief Create a new buffer from raw data.
     * @param data is a pointer to an array of data used to populate DmxBuffer
     * @param length is the length of data in array data
     */
    DmxBuffer(const uint8_t *data, unsigned int length);

    /**
     * @brief Create a new buffer from a string.
     * @param data is a string of raw data values
     *
     * @deprecated Use DmxBuffer(const uint8_t *data, unsigned int length)
     * instead
     */
    explicit DmxBuffer(const string &data);

    /**
     * @brief Destructor
     */
    ~DmxBuffer();

    /**
     * @brief Assignment operator used to make this buffer equal to another
     * buffer.
     * @param other the other DmxBuffer to copy/link from
     */
    DmxBuffer& operator=(const DmxBuffer &other);

    /**
     * @brief Equality operator used to check if two DmxBuffers are equal.
     * @param other is the other DmxBuffer to check against
     * @return true if equal, and false if not
     */
    bool operator==(const DmxBuffer &other) const;

    /**
     * @brief Inequality operator used to check if two DmxBuffers are not equal
     * @param other is the other DmxBuffer to check against
     * @return true if not equal and false if the are equal
     */
    bool operator!=(const DmxBuffer &other) const;

    /**
     * @brief Current size of DmxBuffer
     * @return the current number of slots in the buffer.
     */
    unsigned int Size() const { return m_length; }

    /**
     * @brief HTP Merge from another DmxBuffer.
     * @param other the DmxBuffer to HTP merge into this one
     * @return false if the merge failed, and true if merge was successful
     */
    bool HTPMerge(const DmxBuffer &other);

    /**
     * @brief Set the contents of this DmxBuffer
     * @param data is a pointer to an array of uint8_t values
     * @param length is the size of the array pointed to by data
     * @return true if the set was successful, and false if it failed
     * @post Size() == length
     */
    bool Set(const uint8_t *data, unsigned int length);

    /**
     * @brief Set the contents of this DmxBuffer equal to the string
     * @param data the string with the dmx data
     * @return true if the set was successful and false if it failed
     * @post Size() == data.length()
     */
    bool Set(const string &data);

    /**
     * @brief Sets the data in this buffer to be the same as the other one.
     * This forces a copy of the data rather than using copy-on-write.
     * @param other is another DmxBuffer with data to point to/copy from
     * @return true if the set was successful and false if it failed
     * @post Size() == other.Size()
     */
    bool Set(const DmxBuffer &other);

    /**
     * @brief Set values from a string
     * Convert a comma separated list of values into for the DmxBuffer. Invalid
     * values are set to 0. 0s can be dropped between the commas.
     * @param data the string to split
     * @return true if the set was successful and false if it failed
     *
     * @snippet
     * Here is an example of the string format used:
     * @code
     * dmx_buffer.SetFromString("0,1,2,3,4")
     * @endcode
     * The above code would set channels 1 through 5 to 0,1,2,3,4 respectively,
     * and
     * @code
     * dmx_buffer.SetFromString(",,,,,255,255,128")
     * @endcode
     * would set channel 1 through 5 to 0 and channel 6,7 to 255 and channel
     * 8 to 128.
     */
    bool SetFromString(const string &data);

    /**
     * @brief Set a Range of data to a single value. Calling this on an
     * uninitialized buffer will call Blackout() first. Attempted to set data
     * with an offset
     * greater than Size() is an error
     * @param offset the starting channel
     * @param data is the value to set the range to
     * @param length the length of the range to set
     * @return true if the call successful and false if it failed
     */
    bool SetRangeToValue(unsigned int offset, uint8_t data,
                         unsigned int length);

    /**
     * @brief Set a range of data.
     * Calling this on an uninitialized buffer will call Blackout() first.
     * Attempting to set data with an offset > Size() is an error.
     * @param offset the starting channel
     * @param data a pointer to the new data
     * @param length the length of the data
     * @return true if the call successful and false if it failed
     */
    bool SetRange(unsigned int offset, const uint8_t *data,
                  unsigned int length);

    /**
     * @brief Set a single channel.
     * Calling this on an uninitialized buffer will call Blackout() first.
     * Trying to set a channel more than 1 channel past the end of the valid
     * data is an error.
     * @param channel is the dmx channel you want to change
     * @param data is the value to set channel
     */
    void SetChannel(unsigned int channel, uint8_t data);

    /**
     * @brief Get the contents of this DmxBuffer.
     * This function copies the contents of the DmxBuffer into the memory region
     * pointed to by data.
     * @param data is a pointer to an array to store the data from DmxBuffer
     * @param length is a pointer to the length of data
     * @post *length == Size()
     */
    void Get(uint8_t *data, unsigned int *length) const;

    /**
     * @brief Get a range of values starting from a particular slot.
     * @param slot is the dmx slot to start from
     * @param data is a pointer to where you want to store the gathered data
     * @param length is the length of the data you wish to retrieve
     */
    void GetRange(unsigned int slot, uint8_t *data,
                  unsigned int *length) const;

    /**
     * @brief This function returns the value of a channel, and returns 0 if the
     * buffer wasn't initialized or the channel was out-of-bounds.
     * @param channel is the channel to return
     * @return the value for the requested channel, returns 0 if the channel
     * does not exist
     */
    uint8_t Get(unsigned int channel) const;

    /**
     * @brief Get a raw pointer to the internal data.
     * @return constant pointer to internal data
     */
    const uint8_t *GetRaw() const { return m_data; }

    /**
     * @brief Get the raw contents of the DmxBuffer as a string.
     * @return a string of raw channel values
     */
    string Get() const;

    /**
     * @brief Set the buffer to all zeros.
     * @post Size() == DMX_UNIVERSE_SIZE
     */
    bool Blackout();

    /**
     * @brief Reset the bufer to hold no data.
     * @post Size() == 0
     */
    void Reset();

    /**
     * @brief Convert the DmxBuffer to a human readable representation.
     * @return a string in a human readable form
     *
     * @snippet
     * Here is an example of the output.
     * @code
     * "0,0,255,128,100"
     * @endcode
     */
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

/**
 * @brief Stream operator to allow DmxBuffer to be output to stdout
 * @param out is the output stream
 * @param data is the DmxBuffer to output from
 *
 * @snippet
 * @code
 * DmxBuffer dmx_buffer();
 * cout << dmx_buffer << endl; //Show channel values of
 *                             //dmx_buffer
 * @endcode
 */
std::ostream& operator<<(std::ostream &out, const DmxBuffer &data);
}  // namespace ola
#endif  // INCLUDE_OLA_DMXBUFFER_H_
