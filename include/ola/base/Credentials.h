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
 * @brief Handle getting and setting a process's credentials
 *
 * @addtogroup cred
 * @{
 *
 * @file Credentials.h
 * @brief
 */

#ifndef INCLUDE_OLA_BASE_CREDENTIALS_H_
#define INCLUDE_OLA_BASE_CREDENTIALS_H_

#include <unistd.h>
#include <string>

namespace ola {

using std::string;

// These functions wrap their POSIX counterparts.

/**
 * @brief Gets the real UID of the calling process.
 * @return real user id of the calling proccess
 */
uid_t GetUID();

/**
 * @brief Gets the effective UID of the calling process.
 * @return effective user id of the calling process
 */
uid_t GetEUID();

/**
 * @brief Sets the effective UID of the calling process.
 * @note This is a one way street, and is only valid if the current euid is 0,
 * or euid == new_uid
 * @param new_uid is the user id to set the processes to
 * @return true on success, false otherwise
 */
bool SetUID(uid_t new_uid);

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
 * @brief Set the effective Group ID the process
 * @note This is a one way street. Only valid if the current egid is 0, or
 * egid == new_gid
 * @param new_gid new Group ID to set process group to
 * @return true on success, false otherwise
 */
bool SetGID(gid_t new_gid);

/**
 * @struct PasswdEntry
 * @brief Used as a convient way to hold all the data associated with a
 * specific user or group
 */
typedef struct {
  /**@brief name of the group or user*/
  string pw_name;

  /**@brief Unused currently*/
  string pw_passwd;  // no passwd for now

  /**@brief real User ID*/
  uid_t pw_uid;

  /**@brief real Group ID*/
  gid_t pw_gid;

  /**@brief user's home directory*/
  string pw_dir;

  /**@brief user's shell program*/
  string pw_shell;
} PasswdEntry;

/**
 * @brief Takes in the username of a user and fills a PasswdEntry struct on
 * success
 * @note Only thread safe and reentrant if the underlying OS supports it.
 * Wraps getpwuid/getpwuid_r
 * @param name username to match against
 * @param passwd struct to hold information for username name
 * @return true on success, false otherwise
 */
bool GetPasswdName(const string &name, PasswdEntry *passwd);

/**
 * @brief Takes in the User ID of a user and fills a PasswdEntry struct on
 * success.
 * @note Only thread safe and reentrant if the underlying OS supports it.
 * Wraps getpwuid/getpwuid_r
 * @param uid is the User ID to match against
 * @param passwd struct to hold information for uid
 * @return true on success, false otherwise
 */
bool GetPasswdUID(uid_t uid, PasswdEntry *passwd);

/**
 * @struct GroupEntry
 * @brief Used as a convenient way to  hold information about a group
 */
typedef struct {
  /**@brief name of the group*/
  string gr_name;
  /**
   * @brief password for the group
   * @note UNUSED
   */
  string gr_passwd;  // no passwd for now
  /**@brief Group ID*/
  gid_t gr_gid;
  // vector<string> gr_mem;  // no members for now
} GroupEntry;

/**
 * @brief Takes a name of a group and populates a GroupEntry with information
 * on success.
 * @note This is only thread safe & reentrant if the underlying OS supports it.
 * Wraps getgrpid/getgrpid_r
 * @param name the name of the group to match
 * @param passwd is a GroupEntry to be populated upon success
 * @returns true on success and false otherwise
 */
bool GetGroupName(const string &name, GroupEntry *passwd);

/**
 * @brief Takes a Group ID and populates a GroupEntry with information on
 * success.
 * @note This is only thread safe & reentrant if the underlying OS supports it.
 * Wraps getgrpid/getgrpid_r
 * @param gid is the Group ID to match against
 * @passwd is a GroupEntry to be populated upon success
 * @returns true on success and false otherwise
 */
bool GetGroupGID(gid_t gid, GroupEntry *passwd);
/**@}*/
}  // namespace ola
#endif  // INCLUDE_OLA_BASE_CREDENTIALS_H_
