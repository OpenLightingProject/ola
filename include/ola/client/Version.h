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
 * Version.h
 * Provides version information for the client library.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_CLIENT_VERSION_H_
#define INCLUDE_OLA_CLIENT_VERSION_H_

#include <ola/base/Macro.h>
#include <string>

namespace ola {
namespace client {

#define OLA_CLIENT_VERSION_MAJOR 0
#define OLA_CLIENT_VERSION_MINOR 9
#define OLA_CLIENT_VERSION_REVISION 0

/**
 * @class Version ola/client/Version.h
 * @brief Provides version information.
 * OLA Version numbers are in the form major.minor.revision.
 */
class Version {
 public:
  /**
   * @brief the Major version number
   * @returns the major version number.
   */
  static unsigned int GetMajor();

  /**
   * @brief the Minor version number
   * @returns the minor version number.
   */
  static unsigned int GetMinor();

  /**
   * @brief the revision number
   * @returns the revision number.
   */
  static unsigned int GetRevision();

  /**
   * @brief the version number as a string.
   * @returns the version number as a string.
   */
  static std::string GetVersion();

  /**
   * @brief Check the running version is newer than or equal to a specific
   * version.
   * @param major the major version.
   * @param major the minor version.
   * @param major the revision.
   * @returns true if the running version is >= major.minor,revision.
   */
  static bool IsAtLeast(unsigned int major,
                        unsigned int minor,
                        unsigned int revision);

 private:
  DISALLOW_COPY_AND_ASSIGN(Version);
};
}  // namespace client
}  // namespace ola
#endif  // INCLUDE_OLA_CLIENT_VERSION_H_
