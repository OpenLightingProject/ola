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
 * Serial.cpp
 * Serial I/O Helper methods.
 * Copyright (C) 2014 Peter Newman
 */

#include <string>
#include <vector>

#include "ola/file/Util.h"
#include "ola/io/IOUtils.h"
#include "ola/io/Serial.h"

namespace ola {
namespace io {

using std::vector;
using std::string;

bool UIntToSpeedT(uint32_t value, speed_t *output) {
  switch (value) {
    case BAUD_RATE_9600:
      *output = B9600;
      return true;
    case BAUD_RATE_19200:
      *output = B19200;
      return true;
    case BAUD_RATE_38400:
      *output = B38400;
      return true;
    case BAUD_RATE_57600:
      *output = B57600;
      return true;
    case BAUD_RATE_115200:
      *output = B115200;
      return true;
    case BAUD_RATE_230400:
      *output = B230400;
      return true;
  }
  return false;
}

bool CheckForUUCPLockFile(const std::vector<std::string> &directories,
                          const std::string &serial_device) {
  vector<string>::const_iterator iter = directories.begin();
  for (; iter != directories.end(); ++iter) {
    const string lock_file = (
        *iter + ola::file::PATH_SEPARATOR + "LCK.." + serial_device);
    if (FileExists(lock_file)) {
      return true;
    }
  }
  return false;
}
}  // namespace io
}  // namespace ola
