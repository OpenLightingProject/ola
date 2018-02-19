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
 * OutputBuffer.h
 * An Abstract base class that represents a block of memory that can be written
 * to.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_OUTPUTBUFFER_H_
#define INCLUDE_OLA_IO_OUTPUTBUFFER_H_

#include <stdint.h>

namespace ola {
namespace io {

/**
 * OutputBufferInterface.
 * Output buffers allow appending arbitrary data to the end. An important
 * property of an OutputBuffer is that it doesn't have a fixed length. You can
 * append as much data as you want.
 */
class OutputBufferInterface {
 public:
    virtual ~OutputBufferInterface() {}

    // Returns true if this OutputBuffer is empty.
    virtual bool Empty() const = 0;

    // Return the amount of data in the OutputBuffer
    virtual unsigned int Size() const = 0;

    // Append some data to this OutputBuffer
    virtual void Write(const uint8_t *data, unsigned int length) = 0;
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_OUTPUTBUFFER_H_
