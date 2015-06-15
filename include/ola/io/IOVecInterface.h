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
 * IOVecInterface.h
 * An interface for a class which provides a way of accessing it's data via
 * iovecs. This allows us to write the contents of the class to a network
 * socket without incurring a copy.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_IOVECINTERFACE_H_
#define INCLUDE_OLA_IO_IOVECINTERFACE_H_

#include <stdlib.h>

namespace ola {
namespace io {

/**
 * Portable structure for scatter/gather data. This is binary compatible to
 * iovec on *nix. On Windows, this needs to be converted to WSABUF for socket
 * operations.
 */
struct IOVec {
 public:
    /*
     * Contains the address of a buffer.
     */
    void* iov_base;

    /*
     * Contains the length of the buffer.
     */
    size_t iov_len;
};

/**
 * The interface for an object which can be converted to an array of IOVecs for
 * use with the sendmsg() or WSASendMsg call.
 */
class IOVecInterface {
 public:
    virtual ~IOVecInterface() {}

    /*
     * Returns a pointer to an array of IOVecs and sets io_count to be the
     * number of IOVecs in the array.
     */
    virtual const struct IOVec *AsIOVec(int *io_count) const = 0;

    /**
     * Removes the specified number of bytes from the object.
     */
    virtual void Pop(unsigned int bytes) = 0;

    /*
     * Frees the IOVec array returned by AsIOVec()
     */
    static void FreeIOVec(const struct IOVec *iov) {
      if (iov)
        delete[] iov;
    }
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_IOVECINTERFACE_H_
