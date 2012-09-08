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
 * InputStream.h
 * An Abstract base class that extracts formatted data.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_INPUTSTREAM_H_
#define INCLUDE_OLA_IO_INPUTSTREAM_H_

#include <stdint.h>
#include <ola/io/InputBuffer.h>

namespace ola {
namespace io {

/**
 * InputStream. Similar to istream.
 */
class InputStream {
  public:
    virtual ~InputStream() {}

    /*
     * Extract data from the Stream.
     * Returns false if there was insufficient data to extract.
     */
    virtual bool operator>>(int8_t &val) = 0;
    virtual bool operator>>(uint8_t &val) = 0;
    virtual bool operator>>(int16_t &val) = 0;
    virtual bool operator>>(uint16_t &val) = 0;
    virtual bool operator>>(int32_t &val) = 0;
    virtual bool operator>>(uint32_t &val) = 0;
};
}  // io
}  // ola
#endif  // INCLUDE_OLA_IO_INPUTSTREAM_H_
