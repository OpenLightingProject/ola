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
 * IOUtils.h
 * IO Util functions.
 * Copyright (C) 2013 Hakan Lindestaf
 */

#ifndef INCLUDE_OLA_IO_IOUTILS_H_
#define INCLUDE_OLA_IO_IOUTILS_H_

#include <stdint.h>

#include <string>

namespace ola {
namespace io {

/**
 * @brief Wrapper around open().
 *
 * This logs a message if the open fails.
 * @param path the path to open
 * @param oflag flags passed to open
 * @param[out] fd a pointer to the fd which is returned.
 * @returns true if the open succeeded, false otherwise.
 */
bool Open(const std::string &path, int oflag, int *fd);

/**
 * @brief Wrapper around open().
 *
 * This is similar to Open(), but doesn't log a warning if open() fails.
 * @param path the path to open
 * @param oflag flags passed to open
 * @param[out] fd a pointer to the fd which is returned.
 * @returns true if the open succeeded, false otherwise.
 * @sa Open
 */
bool TryOpen(const std::string &path, int oflag, int *fd);

/**
 * @brief Check if a file exists using stat().
 * @param file_name The name of the file.
 * @returns true if the stat() command completed & the file exists,
 *   false if stat() failed, or the file doesn't exist.
 */
bool FileExists(const std::string &file_name);
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_IOUTILS_H_
