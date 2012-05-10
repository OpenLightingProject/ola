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
 * OutputStream.h
 * A class that you can write binary data to.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_OUTPUTSTREAM_H_
#define INCLUDE_OLA_IO_OUTPUTSTREAM_H_

#include <stdint.h>

namespace ola {
namespace io {

/**
 * OutputStream
 */
class OutputStream {
  public:
    virtual ~OutputStream() {}

    virtual bool Empty() const = 0;

    // The size
    virtual unsigned int Size() const = 0;

    // Append some data to this OutputQueue
    virtual void Write(const uint8_t *data, unsigned int length) = 0;

    virtual OutputStream& operator<<(uint8_t) = 0;
    virtual OutputStream& operator<<(uint16_t) = 0;
    virtual OutputStream& operator<<(uint32_t) = 0;
    virtual OutputStream& operator<<(int8_t) = 0;
    virtual OutputStream& operator<<(int16_t) = 0;
    virtual OutputStream& operator<<(int32_t) = 0;
};
}  // io
}  // ola
#endif  // INCLUDE_OLA_IO_OUTPUTSTREAM_H_
