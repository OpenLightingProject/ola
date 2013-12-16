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
 * Version.cpp
 * Provides version information for the client library.
 * Copyright (C) 2013 Simon Newton
 */

#include "ola/client/Version.h"

#include <sstream>
#include <string>

namespace ola {
namespace client {

using std::string;

unsigned int Version::GetMajor() {
  return OLA_CLIENT_VERSION_MAJOR;
}

unsigned int Version::GetMinor() {
  return OLA_CLIENT_VERSION_MINOR;
}

unsigned int Version::GetRevision() {
  return OLA_CLIENT_VERSION_REVISION;
}

std::string Version::GetVersion() {
  std::ostringstream str;
  str << GetMajor() << "." << GetMinor() << "." << GetRevision();
  return str.str();
}

bool Version::IsAtLeast(unsigned int major,
                        unsigned int minor,
                        unsigned int revision) {
  return (
      GetMajor() >= major &&
      GetMinor() >= minor &&
      GetRevision() >= revision);
}
}  // namespace client
}  // namespace ola
