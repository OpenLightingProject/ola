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
 * InputBuffer.h
 * An Abstract base class that represents a block of memory that can be read
 * from.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_INPUTBUFFER_H_
#define INCLUDE_OLA_IO_INPUTBUFFER_H_

#include <stdint.h>
#include <string>

namespace ola {
namespace io {

/**
 * InputBufferInterface.
 */
class InputBufferInterface {
 public:
    virtual ~InputBufferInterface() {}

    /**
     * Copy the next length bytes to *data. Less than length bytes may be
     * returned if the end of the buffer is reached.
     * @returns the number of bytes read
     */
    virtual unsigned int Read(uint8_t *data, unsigned int length) = 0;

    /*
     * Append up to length bytes of data to the string.
     * Updates length with the number of bytes read.
     * @returns the number of bytes read
     */
    virtual unsigned int Read(std::string *output, unsigned int length) = 0;
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_INPUTBUFFER_H_
