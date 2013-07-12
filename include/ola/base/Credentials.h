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
 * Credentials.h
 * Handle getting and setting a process's credentials.
 * Copyright (C) 2012 Simon Newton
 */

/**
 * @defgroup cred Credentials
 * @brief User & Group Information.
 *
 * @addtogroup cred
 * @{
 *
 * @file Credentials.h
 * @brief User & Group Information
 * @}
 */

#ifndef INCLUDE_OLA_BASE_CREDENTIALS_H_
#define INCLUDE_OLA_BASE_CREDENTIALS_H_

#include <unistd.h>
#include <string>

namespace ola {

/**
 * @addtogroup cred
 * @{
 */
using std::string;

// These functions wrap their POSIX counterparts.

/**
 * @name Get / Set User ID
 * @{
 * */

/**
 * @brief Get the real UID of the process.
 * @return real user id of the proccess
 */
uid_t GetUID();

/**
 * @brief Get the effective UID of the process.
 * @return effective user id of the process
 */
uid_t GetEUID();

/**
 * @brief Set the effective UID of the process.
 * @note This is a one way street, and is only valid if the current euid is 0,
 * or euid == new_uid
 * @param new_uid is the user id to set the processes to
 * @return true on success, false otherwise
 */
bool SetUID(uid_t new_uid);

/**
 * @}
 *
 * @name Get / Set Group ID
 * @{
 */

/**
 * @brief Get the real Group ID
 * @return the real Group ID
 */
gid_t GetGID();

/**
 * @brief Get the effective group ID
 * @return the effective Group ID
 */
gid_t GetEGID();

/**
 * @brief Set the effective Group ID of the process
 * @note This is a one way street. Only valid if the current egid is 0, or
 * egid == new_gid
 * @param new_gid new Group ID to set process group to
 * @return true on success, false otherwise
 */
bool SetGID(gid_t new_gid);

/**
 * @}
 */

/**
 * @struct PasswdEntry
 * @brief Contains information about a user.
 */
typedef struct {
  /** @brief name of the user */
  string pw_name;

  /** @brief Unused currently*/
  string pw_passwd;  // no passwd for now

  /** @brief real User ID */
  uid_t pw_uid;

  /** @brief real Group ID */
  gid_t pw_gid;

  /** @brief user's home directory */
  string pw_dir;

  /** @brief user's shell program */
  string pw_shell;
} PasswdEntry;

/**
 * @name Lookup User Information
 * @{
 */

/**
 *
 * @brief Lookup a user account by username.
 * @note Only thread safe and reentrant if the underlying OS supports it.
 *   @par
 *   Wraps [getpwnam() / getpwnam_r()](
 *   http://man7.org/linux/man-pages/man3/getpwnam.3.html).
 * @param[in] name username to search for.
 * @param[out] passwd struct to hold information for username name
 * @return true on success, false otherwise
 */
bool GetPasswdName(const string &name, PasswdEntry *passwd);

/**
 * @brief Lookup a user account by UID.
 * @note Only thread safe and reentrant if the underlying OS supports it.
 *   @par
 *   Wraps [getpwuid()/getpwuid_r()](
 *   http://man7.org/linux/man-pages/man3/getpwnam.3.html).
 * @param[in] uid is the User ID to match against
 * @param[out] passwd struct to hold information for uid
 * @return true on success, false otherwise
 */
bool GetPasswdUID(uid_t uid, PasswdEntry *passwd);

/**
 * @}
 */

/**
 * @struct GroupEntry
 * @brief Contains information about a group.
 */
typedef struct {
  /** @brief name of the group */
  string gr_name;
  /**
   * @brief password for the group
   * @note UNUSED
   */
  string gr_passwd;  // no passwd for now
  /** @brief Group ID */
  gid_t gr_gid;
  // vector<string> gr_mem;  // no members for now
} GroupEntry;

/**
 * @name Lookup Group Information.
 * @{
 */

/**
 * @brief Lookup a group account by name.
 * @note This is only thread safe & reentrant if the underlying OS supports it.
 *   @par
 *   Wraps [getgrnam()/getgrnam_r()](
 *   http://man7.org/linux/man-pages/man3/getgrgid.3.html).
 * @param[in] name the name of the group to match
 * @param[out] passwd is a GroupEntry to be populated upon success
 * @returns true on success and false otherwise
 */
bool GetGroupName(const string &name, GroupEntry *passwd);

/**
 * @brief Lookup a group account by GID.
 * @note This is only thread safe & reentrant if the underlying OS supports it.
 *   @par
 *   Wraps [getgrpid()/getgrpid_r()](
 *   http://man7.org/linux/man-pages/man3/getgrgid.3.html).
 * @param[in] gid is the Group ID to match against
 * @param[out] passwd is a GroupEntry to be populated upon success
 * @returns true on success and false otherwise
 */
bool GetGroupGID(gid_t gid, GroupEntry *passwd);
/**
 * @}
 * @}
 */
}  // namespace ola
#endif  // INCLUDE_OLA_BASE_CREDENTIALS_H_
