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
 * IOUtils.cpp
 * I/O Helper methods.
 * Copyright (C) 2013 Simon Newton
 */

#include "ola/io/IOUtils.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <string>

#include "ola/Logging.h"

namespace ola {
namespace io {

using std::string;

bool Open(const string &path, int oflag, int *fd) {
  *fd = open(path.c_str(), oflag);
  if (*fd < 0) {
    OLA_WARN << "open(" << path << "): " << strerror(errno);
    return false;
  }
  return true;
}

bool TryOpen(const string &path, int oflag, int *fd) {
  *fd = open(path.c_str(), oflag);
  if (*fd < 0) {
    OLA_INFO << "open(" << path << "): " << strerror(errno);
  }
  return *fd >= 0;
}

bool FileExists(const std::string &file_name) {
  struct stat file_stat;
  return 0 == stat(file_name.c_str(), &file_stat);
}
}  // namespace io
}  // namespace ola
